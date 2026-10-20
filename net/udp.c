#include "net.h"
#include "printk.h"
#include "string.h"

#define UDP_SLOTS 4
#define UDP_PAY 512

struct udp_mbox {
    int used;
    uint16_t port;
    uint8_t data[UDP_PAY];
    uint16_t len;
    uint8_t src_ip[4];
    uint16_t src_port;
};

static struct udp_mbox box[UDP_SLOTS];

int udp_send(const uint8_t dst_ip[4], uint16_t sport, uint16_t dport,
             const void *data, uint16_t len) {
    if (!dst_ip || !data)
        return -1;
    if (len > 1400)
        len = 1400;

    uint8_t buf[1500];
    struct udp_hdr *u = (struct udp_hdr *)buf;
    uint8_t my_ip[4];
    net_get_ip(my_ip);

    u->sport = net_htons(sport);
    u->dport = net_htons(dport);
    u->len = net_htons((uint16_t)(sizeof(struct udp_hdr) + len));
    u->checksum = 0;
    memcpy(buf + sizeof(struct udp_hdr), data, len);

    uint16_t ulen = (uint16_t)(sizeof(struct udp_hdr) + len);
    u->checksum =
        net_htons(net_checksum_tcpudp(my_ip, dst_ip, IP_PROTO_UDP, buf, ulen));
    if (u->checksum == 0)
        u->checksum = 0xFFFF;

    return ipv4_send(dst_ip, IP_PROTO_UDP, buf, ulen);
}

void udp_input(const uint8_t *pkt, uint16_t len, const uint8_t src_ip[4],
               const uint8_t dst_ip[4]) {
    (void)dst_ip;
    if (!pkt || len < sizeof(struct udp_hdr))
        return;
    const struct udp_hdr *u = (const struct udp_hdr *)pkt;
    uint16_t dport = net_ntohs(u->dport);
    uint16_t sport = net_ntohs(u->sport);
    uint16_t ulen = net_ntohs(u->len);
    if (ulen < sizeof(struct udp_hdr) || ulen > len)
        return;
    uint16_t plen = (uint16_t)(ulen - sizeof(struct udp_hdr));
    const uint8_t *payload = pkt + sizeof(struct udp_hdr);

    for (int i = 0; i < UDP_SLOTS; i++) {
        if (box[i].used && box[i].port == dport && box[i].len == 0) {
            if (plen > UDP_PAY)
                plen = UDP_PAY;
            memcpy(box[i].data, payload, plen);
            box[i].len = plen;
            memcpy(box[i].src_ip, src_ip, 4);
            box[i].src_port = sport;
            return;
        }
    }
    /* opportunistic store first free */
    for (int i = 0; i < UDP_SLOTS; i++) {
        if (!box[i].used) {
            box[i].used = 1;
            box[i].port = dport;
            if (plen > UDP_PAY)
                plen = UDP_PAY;
            memcpy(box[i].data, payload, plen);
            box[i].len = plen;
            memcpy(box[i].src_ip, src_ip, 4);
            box[i].src_port = sport;
            return;
        }
    }
}

int udp_recv(uint16_t bind_port, uint8_t *buf, uint16_t max, uint8_t src_ip[4],
             uint16_t *src_port, int polls) {
    int slot = -1;
    for (int i = 0; i < UDP_SLOTS; i++) {
        if (box[i].used && box[i].port == bind_port) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        for (int i = 0; i < UDP_SLOTS; i++) {
            if (!box[i].used) {
                box[i].used = 1;
                box[i].port = bind_port;
                box[i].len = 0;
                slot = i;
                break;
            }
        }
    }
    if (slot < 0)
        return -1;

    for (int t = 0; t < polls; t++) {
        net_poll();
        if (box[slot].len > 0) {
            uint16_t n = box[slot].len;
            if (n > max)
                n = max;
            memcpy(buf, box[slot].data, n);
            if (src_ip)
                memcpy(src_ip, box[slot].src_ip, 4);
            if (src_port)
                *src_port = box[slot].src_port;
            box[slot].len = 0;
            return (int)n;
        }
        for (volatile int i = 0; i < 20000; i++)
            ;
    }
    return 0;
}
