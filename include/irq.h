#ifndef SIF_IRQ_H
#define SIF_IRQ_H

#include <stdint.h>

typedef void (*irq_handler_t)(void);

void irq_init(void);
void irq_install_handler(uint8_t irq, irq_handler_t handler);
void irq_uninstall_handler(uint8_t irq);
void irq_dispatch(uint8_t irq);

#endif
