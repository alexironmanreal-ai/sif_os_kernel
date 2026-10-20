#ifndef SIF_NE2K_H
#define SIF_NE2K_H
#include <stdint.h>
int ne2k_init(void);
int ne2k_present(void);
void ne2k_get_mac(uint8_t out[6]);
int ne2k_send(const void *data, uint16_t len);
int ne2k_recv(void *buf, uint16_t max);
#endif
