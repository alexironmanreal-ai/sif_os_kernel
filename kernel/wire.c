#include "wire.h"
#include "net.h"
#include "ne2k.h"
#include "printk.h"
#include "string.h"

/* thin compat layer over net_ + experimental SIF ethertype */

void wire_init(void) {
    /* net_init owns NE2000 */
}

int wire_up(void) { return net_up(); }

void wire_mac(uint8_t mac[6]) { net_get_mac(mac); }

uint32_t wire_tx_count(void) { return 0; }
uint32_t wire_rx_count(void) { return 0; }

void wire_status(void) { net_status(); }

int wire_cast(const uint8_t dst[6], const void *payload, uint16_t len) {
    return eth_send(dst, WIRE_ETHTYPE_SIF, payload, len);
}

int wire_cast_text(const char *text) {
    if (!text)
        return -1;
    uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint16_t n = 0;
    while (text[n] && n < 200)
        n++;
    return wire_cast(bcast, text, n);
}

int wire_sniff(int verbose) {
    net_poll();
    if (verbose)
        printk("(poll done — use net/arp/ping)\n");
    return 0;
}
