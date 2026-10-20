#include "net.h"
#include "printk.h"
#include "string.h"

enum { TC_CLOSED, TC_SYN_SENT, TC_EST, TC_FIN_WAIT };

static int state = TC_CLOSED;
static uint8_t peer_ip[4];
static uint16_t local_port = 40000;
static uint16_t peer_port;
static uint32_t snd_nxt;
static uint32_t rcv_nxt;
static uint8_t rxbuf[1024];
static uint16_t rxlen;

int tcp_connected(void) { return state == TC_EST; }

static int tcp_xmit(uint8_t flags, const void *data, uint16_t dlen) {
    uint8_t buf[1500];
    struct tcp_hdr *t = (struct tcp_hdr *)buf;
    uint8_t my_ip[4];
    net_get_ip(my_ip);
    memset(t, 0, sizeof(*t));
    t->sport = net_htons(local_port);
    t->dport = net_htons(peer_port);
    t->seq = net_htonl(snd_nxt);
    t->ack = net_htonl(rcv_nxt);
    t->off_res = (5 << 4);
    t->flags = flags;
    t->win = net_htons(4096);
    t->checksum = 0;
    if (data && dlen)
        memcpy(buf + sizeof(struct tcp_hdr), data, dlen);
    uint16_t tlen = (uint16_t)(sizeof(struct tcp_hdr) + dlen);
    t->checksum = net_htons(
        net_checksum_tcpudp(my_ip, peer_ip, IP_PROTO_TCP, buf, tlen));
    int r = ipv4_send(peer_ip, IP_PROTO_TCP, buf, tlen);
    if (r >= 0) {
        if (flags & TCP_SYN)
            snd_nxt++;
        if (flags & TCP_FIN)
            snd_nxt++;
        snd_nxt += dlen;
    }
    return r;
}

int tcp_connect(const uint8_t dst_ip[4], uint16_t dport) {
    if (!dst_ip)
        return -1;
    memcpy(peer_ip, dst_ip, 4);
    peer_port = dport;
    local_port = (uint16_t)(40000 + (dport & 0xFF));
    snd_nxt = 1000;
    rcv_nxt = 0;
    rxlen = 0;
    state = TC_SYN_SENT;
    if (tcp_xmit(TCP_SYN, 0, 0) < 0) {
        state = TC_CLOSED;
        return -1;
    }
    for (int t = 0; t < 40; t++) {
        for (volatile int i = 0; i < 40000; i++)
            ;
        net_poll();
        if (state == TC_EST)
            return 0;
    }
    state = TC_CLOSED;
    printk("[tcp] connect timeout\n");
    return -1;
}

void tcp_input(const uint8_t *pkt, uint16_t len, const uint8_t src_ip[4],
               const uint8_t dst_ip[4]) {
    (void)dst_ip;
    if (!pkt || len < sizeof(struct tcp_hdr))
        return;
    const struct tcp_hdr *t = (const struct tcp_hdr *)pkt;
    uint16_t sport = net_ntohs(t->sport);
    uint16_t dport = net_ntohs(t->dport);
    uint32_t seq = net_ntohl(t->seq);
    uint32_t ack = net_ntohl(t->ack);
    uint8_t flags = t->flags;
    uint8_t doff = (uint8_t)((t->off_res >> 4) * 4);
    if (doff < 20 || len < doff)
        return;

    if (dport != local_port || memcmp(src_ip, peer_ip, 4) != 0)
        return;

    if (state == TC_SYN_SENT && (flags & TCP_SYN) && (flags & TCP_ACK)) {
        rcv_nxt = seq + 1;
        snd_nxt = ack;
        tcp_xmit(TCP_ACK, 0, 0);
        state = TC_EST;
        printk("[tcp] ESTABLISHED %u.%u.%u.%u:%u\n", peer_ip[0], peer_ip[1],
               peer_ip[2], peer_ip[3], peer_port);
        return;
    }

    if (state != TC_EST)
        return;

    uint16_t plen = (uint16_t)(len - doff);
    const uint8_t *payload = pkt + doff;

    if (flags & TCP_RST) {
        state = TC_CLOSED;
        return;
    }

    if (plen > 0 && seq == rcv_nxt) {
        uint16_t n = plen;
        if (n > (uint16_t)(sizeof(rxbuf) - rxlen))
            n = (uint16_t)(sizeof(rxbuf) - rxlen);
        memcpy(rxbuf + rxlen, payload, n);
        rxlen = (uint16_t)(rxlen + n);
        rcv_nxt += plen;
        tcp_xmit(TCP_ACK, 0, 0);
    }

    if (flags & TCP_FIN) {
        rcv_nxt = seq + 1;
        tcp_xmit(TCP_ACK | TCP_FIN, 0, 0);
        state = TC_CLOSED;
    }

    (void)sport;
    (void)ack;
}

int tcp_send(const void *data, uint16_t len) {
    if (state != TC_EST || !data)
        return -1;
    return tcp_xmit(TCP_ACK | TCP_PSH, data, len);
}

int tcp_recv(uint8_t *buf, uint16_t max, int polls) {
    if (!buf)
        return -1;
    for (int t = 0; t < polls; t++) {
        net_poll();
        if (rxlen > 0) {
            uint16_t n = rxlen;
            if (n > max)
                n = max;
            memcpy(buf, rxbuf, n);
            if (n < rxlen)
                memmove(rxbuf, rxbuf + n, rxlen - n);
            rxlen = (uint16_t)(rxlen - n);
            return (int)n;
        }
        if (state == TC_CLOSED && rxlen == 0)
            return 0;
        for (volatile int i = 0; i < 30000; i++)
            ;
    }
    return 0;
}

void tcp_close(void) {
    if (state == TC_EST)
        tcp_xmit(TCP_FIN | TCP_ACK, 0, 0);
    state = TC_CLOSED;
    rxlen = 0;
}
