#include "printk.h"
#include "vga.h"
#include <stdarg.h>
#include <stdint.h>

static void print_uint(unsigned long n, unsigned base) {
    char buf[32];
    int i = 0;
    if (n == 0) { vga_putc('0'); return; }
    while (n) {
        unsigned d = n % base;
        buf[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        n /= base;
    }
    while (i--) vga_putc(buf[i]);
}

void printk(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    for (; *fmt; fmt++) {
        if (*fmt != '%') { vga_putc(*fmt); continue; }
        fmt++;
        switch (*fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            vga_write(s ? s : "(null)");
            break;
        }
        case 'd': {
            int v = va_arg(ap, int);
            if (v < 0) { vga_putc('-'); v = -v; }
            print_uint((unsigned)v, 10);
            break;
        }
        case 'u':
            print_uint(va_arg(ap, unsigned), 10);
            break;
        case 'x':
            vga_write("0x");
            print_uint(va_arg(ap, unsigned), 16);
            break;
        case 'c':
            vga_putc((char)va_arg(ap, int));
            break;
        case '%':
            vga_putc('%');
            break;
        default:
            vga_putc('%');
            vga_putc(*fmt);
            break;
        }
    }
    va_end(ap);
}
