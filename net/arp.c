#include "net.h"
#include "printk.h"
#include "string.h"

#define ARP_TABLE 16
#define ARP_REQUEST 1
#define ARP_REPLY   2

struct arp_entry {
    uint8_t ip[4];
    uint8_t mac[6];
    int used;
};

static struct arp_entry table[ARP_TABLE];

static void arp_learn(const uint8_t ip[4], const uint8_t mac[6]) {
    for (int i = 0; i < ARP_TABLE; i++) {
        if (table[i].used && memcmp(table[i].ip, ip, 4) == 0) {
            memcpy(table[i].mac, mac, 6);
            return;
        }
    }
    for (int i = 0; i < ARP_TABLE; i++) {
        if (!table[i].used) {
            table[i].used = 1;
            memcpy(table[i].ip, ip, 4);
            memcpy(table[i].mac, mac, 6);
            return;
        }
    }
    /* overwrite slot 0 */
    table[0].used = 1;
    memcpy(table[0].ip, ip, 4);
    memcpy(table[0].mac, mac, 6);
}

int arp_lookup(const uint8_t ip[4], uint8_t mac_out[6]) {
    for (int i = 0; i < ARP_TABLE; i++) {
        if (table[i].used && memcmp(table[i].ip, ip, 4) == 0) {
            memcpy(mac_out, table[i].mac, 6);
            return 0;
        }
    }
    return -1;
}

int arp_request(const uint8_t tip[4]) {
    uint8_t my_mac[6], my_ip[4];
    net_get_mac(my_mac);
    net_get_ip(my_ip);

    struct arp_pkt a;
    memset(&a, 0, sizeof(a));
    a.htype = net_htons(1);
    a.ptype = net_htons(ETH_TYPE_IPV4);
    a.hlen = 6;
    a.plen = 4;
    a.oper = net_htons(ARP_REQUEST);
    memcpy(a.sha, my_mac, 6);
    memcpy(a.spa, my_ip, 4);
    memset(a.tha, 0, 6);
    memcpy(a.tpa, tip, 4);

    uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    return eth_send(bcast, ETH_TYPE_ARP, &a, sizeof(a));
}

void arp_input(const uint8_t *pkt, uint16_t len, const uint8_t src_mac[6]) {
    if (!pkt || len < sizeof(struct arp_pkt))
        return;
    const struct arp_pkt *a = (const struct arp_pkt *)pkt;
    if (net_ntohs(a->htype) != 1 || net_ntohs(a->ptype) != ETH_TYPE_IPV4)
        return;
    if (a->hlen != 6 || a->plen != 4)
        return;

    uint16_t op = net_ntohs(a->oper);
    arp_learn(a->spa, a->sha);
    (void)src_mac;

    uint8_t my_ip[4], my_mac[6];
    net_get_ip(my_ip);
    net_get_mac(my_mac);

    if (op == ARP_REQUEST && memcmp(a->tpa, my_ip, 4) == 0) {
        struct arp_pkt r;
        memset(&r, 0, sizeof(r));
        r.htype = net_htons(1);
        r.ptype = net_htons(ETH_TYPE_IPV4);
        r.hlen = 6;
        r.plen = 4;
        r.oper = net_htons(ARP_REPLY);
        memcpy(r.sha, my_mac, 6);
        memcpy(r.spa, my_ip, 4);
        memcpy(r.tha, a->sha, 6);
        memcpy(r.tpa, a->spa, 4);
        eth_send(a->sha, ETH_TYPE_ARP, &r, sizeof(r));
        printk("[arp] reply to %u.%u.%u.%u\n", a->spa[0], a->spa[1], a->spa[2],
               a->spa[3]);
    }
}

void arp_dump(void) {
    printk("ARP table:\n");
    int n = 0;
    for (int i = 0; i < ARP_TABLE; i++) {
        if (!table[i].used)
            continue;
        n++;
        printk("  %u.%u.%u.%u -> %02x:%02x:%02x:%02x:%02x:%02x\n",
               table[i].ip[0], table[i].ip[1], table[i].ip[2], table[i].ip[3],
               table[i].mac[0], table[i].mac[1], table[i].mac[2],
               table[i].mac[3], table[i].mac[4], table[i].mac[5]);
    }
    if (!n)
        printk("  (empty)\n");
}
