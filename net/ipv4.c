#include "net.h"
#include "printk.h"
#include "string.h"

static uint16_t ip_id;

int ipv4_send(const uint8_t dst_ip[4], uint8_t proto, const void *payload,
              uint16_t len) {
    if (!dst_ip || !payload)
        return -1;
    if (len > 1400)
        len = 1400;

    uint8_t mac[6];
    if (arp_lookup(dst_ip, mac) < 0) {
        arp_request(dst_ip);
        /* one poll hope for reply in QEMU */
        for (volatile int i = 0; i < 50000; i++)
            ;
        net_poll();
        if (arp_lookup(dst_ip, mac) < 0) {
            /* try gateway for off-link (simple: always try gw ARP if fail) */
            uint8_t gw[4] = {10, 0, 2, 2};
            if (memcmp(dst_ip, gw, 4) != 0) {
                if (arp_lookup(gw, mac) < 0) {
                    arp_request(gw);
                    for (volatile int i = 0; i < 50000; i++)
                        ;
                    net_poll();
                    if (arp_lookup(gw, mac) < 0)
                        return -1;
                }
            } else
                return -1;
        }
    }

    uint8_t buf[1500];
    struct ip_hdr *ip = (struct ip_hdr *)buf;
    uint8_t my_ip[4];
    net_get_ip(my_ip);

    memset(ip, 0, sizeof(*ip));
    ip->ver_ihl = 0x45;
    ip->tos = 0;
    ip->total_len = net_htons((uint16_t)(sizeof(struct ip_hdr) + len));
    ip->id = net_htons(++ip_id);
    ip->frag = 0;
    ip->ttl = 64;
    ip->proto = proto;
    ip->checksum = 0;
    memcpy(ip->src, my_ip, 4);
    memcpy(ip->dst, dst_ip, 4);
    ip->checksum = net_htons(net_checksum(ip, sizeof(struct ip_hdr)));

    memcpy(buf + sizeof(struct ip_hdr), payload, len);
    return eth_send(mac, ETH_TYPE_IPV4, buf,
                    (uint16_t)(sizeof(struct ip_hdr) + len));
}

void ipv4_input(const uint8_t *pkt, uint16_t len) {
    if (!pkt || len < sizeof(struct ip_hdr))
        return;
    const struct ip_hdr *ip = (const struct ip_hdr *)pkt;
    if ((ip->ver_ihl >> 4) != 4)
        return;
    uint8_t ihl = (uint8_t)((ip->ver_ihl & 0x0F) * 4);
    if (ihl < 20 || len < ihl)
        return;

    uint8_t my_ip[4];
    net_get_ip(my_ip);
    if (memcmp(ip->dst, my_ip, 4) != 0) {
        /* ignore non-us (no promisc) */
        int bcast = (ip->dst[0] == 255);
        if (!bcast)
            return;
    }

    const uint8_t *payload = pkt + ihl;
    uint16_t plen = (uint16_t)(len - ihl);

    if (ip->proto == IP_PROTO_ICMP)
        icmp_input(payload, plen, ip->src);
    /* UDP/TCP later */
}
