#ifndef SIF_NET_H
#define SIF_NET_H

#include <stdint.h>

#define ETH_TYPE_IPV4 0x0800
#define ETH_TYPE_ARP  0x0806

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

struct eth_hdr {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t type; /* big-endian on wire */
} __attribute__((packed));

struct arp_pkt {
    uint16_t htype;
    uint16_t ptype;
    uint8_t  hlen;
    uint8_t  plen;
    uint16_t oper;
    uint8_t  sha[6];
    uint8_t  spa[4];
    uint8_t  tha[6];
    uint8_t  tpa[4];
} __attribute__((packed));

struct ip_hdr {
    uint8_t  ver_ihl;
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t frag;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint8_t  src[4];
    uint8_t  dst[4];
} __attribute__((packed));

struct icmp_hdr {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} __attribute__((packed));

uint16_t net_htons(uint16_t x);
uint16_t net_ntohs(uint16_t x);
uint32_t net_htonl(uint32_t x);
uint16_t net_checksum(const void *data, uint16_t len);

void net_init(void);
int net_up(void);
void net_poll(void); /* RX process one/several frames */
void net_status(void);

void net_set_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
void net_get_ip(uint8_t out[4]);
void net_get_mac(uint8_t out[6]);

/* Ethernet */
int eth_send(const uint8_t dst[6], uint16_t ethertype, const void *payload,
             uint16_t len);
void eth_input(const uint8_t *frame, uint16_t len);

/* ARP */
void arp_input(const uint8_t *pkt, uint16_t len, const uint8_t src_mac[6]);
int arp_lookup(const uint8_t ip[4], uint8_t mac_out[6]);
int arp_request(const uint8_t tip[4]);
void arp_dump(void);

/* IPv4 */
int ipv4_send(const uint8_t dst_ip[4], uint8_t proto, const void *payload,
              uint16_t len);
void ipv4_input(const uint8_t *pkt, uint16_t len);

/* ICMP */
void icmp_input(const uint8_t *pkt, uint16_t len, const uint8_t src_ip[4]);
int icmp_ping(const uint8_t dst_ip[4], uint16_t seq);

/* parse "a.b.c.d" → 4 bytes; return 0 ok */
int net_parse_ip(const char *s, uint8_t out[4]);

#endif
