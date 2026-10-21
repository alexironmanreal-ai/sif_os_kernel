#include "serial.h"
#include "io.h"

void serial_init(void) {
    outb(COM1 + 1, 0x00); /* disable interrupts */
    outb(COM1 + 3, 0x80); /* DLAB on */
    outb(COM1 + 0, 0x03); /* baud 38400 divisor */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03); /* 8N1 */
    outb(COM1 + 2, 0xC7); /* FIFO */
    outb(COM1 + 4, 0x0B); /* IRQs enabled, RTS/DSR */
    outb(COM1 + 1, 0x01); /* enable received-data IRQ (optional) */
}

int serial_ready(void) {
    return inb(COM1 + 5) & 0x20;
}

void serial_putc(char c) {
    if (c == '\n')
        serial_putc('\r');
    while (!serial_ready())
        ;
    outb(COM1 + 0, (uint8_t)c);
}

void serial_write(const char *s) {
    while (*s)
        serial_putc(*s++);
}

int serial_has_input(void) {
    return (inb(COM1 + 5) & 0x01) != 0;
}

int serial_getc(void) {
    if (!serial_has_input())
        return 0;
    return (int)inb(COM1 + 0);
}
