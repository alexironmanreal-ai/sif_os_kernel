#include "net.h"
#include "printk.h"
#include "string.h"

#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO       8

static uint16_t ping_id = 0x5349; /* 'SI' */
static volatile int ping_got;
static uint16_t ping_seq_got;

void icmp_input(const uint8_t *pkt, uint16_t len, const uint8_t src_ip[4]) {
    if (!pkt || len < sizeof(struct icmp_hdr))
        return;
    const struct icmp_hdr *ic = (const struct icmp_hdr *)pkt;

    if (ic->type == ICMP_ECHO) {
        /* reply */
        uint8_t buf[1500];
        if (len > 1400)
            len = 1400;
        memcpy(buf, pkt, len);
        struct icmp_hdr *r = (struct icmp_hdr *)buf;
        r->type = ICMP_ECHO_REPLY;
        r->code = 0;
        r->checksum = 0;
        r->checksum = net_htons(net_checksum(buf, len));
        ipv4_send(src_ip, IP_PROTO_ICMP, buf, len);
        printk("[icmp] echo → %u.%u.%u.%u\n", src_ip[0], src_ip[1], src_ip[2],
               src_ip[3]);
        return;
    }

    if (ic->type == ICMP_ECHO_REPLY) {
        uint16_t id = net_ntohs(ic->id);
        uint16_t seq = net_ntohs(ic->seq);
        if (id == ping_id) {
            ping_got = 1;
            ping_seq_got = seq;
            printk("[icmp] reply seq=%u from %u.%u.%u.%u\n", seq, src_ip[0],
                   src_ip[1], src_ip[2], src_ip[3]);
        }
    }
}

int icmp_ping(const uint8_t dst_ip[4], uint16_t seq) {
    uint8_t buf[64];
    memset(buf, 0, sizeof(buf));
    struct icmp_hdr *ic = (struct icmp_hdr *)buf;
    ic->type = ICMP_ECHO;
    ic->code = 0;
    ic->id = net_htons(ping_id);
    ic->seq = net_htons(seq);
    for (int i = 0; i < 32; i++)
        buf[sizeof(struct icmp_hdr) + i] = (uint8_t)('0' + (i % 10));
    uint16_t len = (uint16_t)(sizeof(struct icmp_hdr) + 32);
    ic->checksum = 0;
    ic->checksum = net_htons(net_checksum(buf, len));

    ping_got = 0;
    if (ipv4_send(dst_ip, IP_PROTO_ICMP, buf, len) < 0) {
        printk("ping: send fail (ARP?)\n");
        return -1;
    }

    /* poll for reply */
    for (int t = 0; t < 20; t++) {
        for (volatile int i = 0; i < 100000; i++)
            ;
        net_poll();
        if (ping_got && ping_seq_got == seq)
            return 0;
    }
    printk("ping: timeout seq=%u\n", seq);
    return -1;
}
