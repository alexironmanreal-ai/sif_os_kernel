#ifndef SIF_SERIAL_H
#define SIF_SERIAL_H

#include <stdint.h>

#define COM1 0x3F8

void serial_init(void);
void serial_putc(char c);
void serial_write(const char *s);
int  serial_ready(void);

#endif
