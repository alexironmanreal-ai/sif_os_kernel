#ifndef SIF_WIRE_H
#define SIF_WIRE_H

#include <stdint.h>

/*
 * SIF Wire — own link layer naming (not sockets/POSIX).
 * Frames on the ether via NE2000.
 */

#define WIRE_ETHTYPE_SIF 0x88B5 /* locally administered experimental */

void wire_init(void);
int wire_up(void);
void wire_mac(uint8_t mac[6]);
void wire_status(void);

/* send payload as SIF ethertype broadcast (or to dst mac) */
int wire_cast(const uint8_t dst[6], const void *payload, uint16_t len);
int wire_cast_text(const char *text);

/* try receive one frame; print summary; return bytes or 0 */
int wire_sniff(int verbose);

uint32_t wire_tx_count(void);
uint32_t wire_rx_count(void);

#endif
