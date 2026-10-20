#include "net.h"
#include "ne2k.h"
#include "printk.h"
#include "string.h"
#include "dev.h"

static int up;
static uint8_t my_mac[6];
static uint8_t my_ip[4] = {10, 0, 2, 15};
static uint8_t my_mask[4] = {255, 255, 255, 0};
static uint8_t my_gw[4] = {10, 0, 2, 2};

uint16_t net_htons(uint16_t x) { return (uint16_t)((x << 8) | (x >> 8)); }
uint16_t net_ntohs(uint16_t x) { return net_htons(x); }

uint32_t net_htonl(uint32_t x) {
    return ((x & 0xFF000000u) >> 24) | ((x & 0x00FF0000u) >> 8) |
           ((x & 0x0000FF00u) << 8) | ((x & 0x000000FFu) << 24);
}
uint32_t net_ntohl(uint32_t x) { return net_htonl(x); }

uint16_t net_checksum(const void *data, uint16_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint32_t sum = 0;
    while (len > 1) {
        sum += (uint32_t)((p[0] << 8) | p[1]);
        p += 2;
        len = (uint16_t)(len - 2);
    }
    if (len)
        sum += (uint32_t)(p[0] << 8);
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)(~sum);
}

uint16_t net_checksum_tcpudp(const uint8_t src[4], const uint8_t dst[4],
                             uint8_t proto, const void *data, uint16_t len) {
    uint32_t sum = 0;
    sum += (uint32_t)((src[0] << 8) | src[1]);
    sum += (uint32_t)((src[2] << 8) | src[3]);
    sum += (uint32_t)((dst[0] << 8) | dst[1]);
    sum += (uint32_t)((dst[2] << 8) | dst[3]);
    sum += (uint32_t)proto;
    sum += (uint32_t)len;
    const uint8_t *p = (const uint8_t *)data;
    uint16_t n = len;
    while (n > 1) {
        sum += (uint32_t)((p[0] << 8) | p[1]);
        p += 2;
        n = (uint16_t)(n - 2);
    }
    if (n)
        sum += (uint32_t)(p[0] << 8);
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)(~sum);
}

int net_parse_ip(const char *s, uint8_t out[4]) {
    if (!s || !out)
        return -1;
    int part = 0, val = 0, digits = 0;
    for (;; s++) {
        if (*s >= '0' && *s <= '9') {
            val = val * 10 + (*s - '0');
            if (val > 255)
                return -1;
            digits++;
        } else if (*s == '.' || *s == 0) {
            if (!digits || part > 3)
                return -1;
            out[part++] = (uint8_t)val;
            val = 0;
            digits = 0;
            if (*s == 0)
                break;
        } else
            return -1;
    }
    return part == 4 ? 0 : -1;
}

void net_init(void) {
    up = 0;
    if (ne2k_init() != 0) {
        dev_register("eth0", DEV_KIND_LINK, 0, "no NIC");
        printk("[net] DOWN\n");
        return;
    }
    ne2k_get_mac(my_mac);
    up = 1;
    dev_register("eth0", DEV_KIND_LINK, 1, "NE2000 eth");
    printk("[net] UP %u.%u.%u.%u\n", my_ip[0], my_ip[1], my_ip[2], my_ip[3]);
}

int net_up(void) { return up; }
void net_get_mac(uint8_t out[6]) {
    for (int i = 0; i < 6; i++)
        out[i] = my_mac[i];
}
void net_get_ip(uint8_t out[4]) {
    for (int i = 0; i < 4; i++)
        out[i] = my_ip[i];
}
void net_set_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    my_ip[0] = a;
    my_ip[1] = b;
    my_ip[2] = c;
    my_ip[3] = d;
}

void net_status(void) {
    if (!up) {
        printk("net: DOWN\n");
        return;
    }
    printk("net: UP eth0\n");
    printk("  mac  %02x:%02x:%02x:%02x:%02x:%02x\n", my_mac[0], my_mac[1],
           my_mac[2], my_mac[3], my_mac[4], my_mac[5]);
    printk("  ip   %u.%u.%u.%u\n", my_ip[0], my_ip[1], my_ip[2], my_ip[3]);
    printk("  mask %u.%u.%u.%u\n", my_mask[0], my_mask[1], my_mask[2],
           my_mask[3]);
    printk("  gw   %u.%u.%u.%u\n", my_gw[0], my_gw[1], my_gw[2], my_gw[3]);
    printk("  tcp  %s\n", tcp_connected() ? "ESTABLISHED" : "CLOSED");
}

int eth_send(const uint8_t dst[6], uint16_t ethertype, const void *payload,
             uint16_t len) {
    if (!up || !dst || !payload)
        return -1;
    if (len > 1500)
        len = 1500;
    uint8_t frame[1514];
    memset(frame, 0, sizeof(frame));
    for (int i = 0; i < 6; i++) {
        frame[i] = dst[i];
        frame[6 + i] = my_mac[i];
    }
    frame[12] = (uint8_t)(ethertype >> 8);
    frame[13] = (uint8_t)(ethertype & 0xFF);
    memcpy(frame + 14, payload, len);
    uint16_t total = (uint16_t)(14 + len);
    if (total < 60)
        total = 60;
    return ne2k_send(frame, total);
}

void eth_input(const uint8_t *frame, uint16_t len) {
    if (!frame || len < 14)
        return;
    uint16_t type = (uint16_t)((frame[12] << 8) | frame[13]);
    if (type == ETH_TYPE_ARP)
        arp_input(frame + 14, (uint16_t)(len - 14), frame + 6);
    else if (type == ETH_TYPE_IPV4)
        ipv4_input(frame + 14, (uint16_t)(len - 14));
}

void net_poll(void) {
    if (!up)
        return;
    uint8_t buf[1514];
    for (int i = 0; i < 16; i++) {
        int n = ne2k_recv(buf, sizeof(buf));
        if (n <= 0)
            break;
        eth_input(buf, (uint16_t)n);
    }
}
