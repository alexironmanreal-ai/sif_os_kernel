#include "serial.h"
#include "io.h"

void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
    outb(COM1 + 4, 0x1E);
    outb(COM1 + 0, 0xAE);
    if (inb(COM1 + 0) != 0xAE) { }
    outb(COM1 + 4, 0x0F);
}

int serial_ready(void) {
    return inb(COM1 + 5) & 0x20;
}

void serial_putc(char c) {
    if (c == '\n') serial_putc('\r');
    while (!serial_ready()) { }
    outb(COM1 + 0, (uint8_t)c);
}

void serial_write(const char *s) {
    while (*s) serial_putc(*s++);
}
