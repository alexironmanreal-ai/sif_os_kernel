#include "printk.h"
#include "vga.h"
#include "serial.h"
#include <stdarg.h>
#include <stdint.h>

static void kputc(char c) {
    vga_putc(c);
    serial_putc(c);
}
static void kwrite(const char *s) {
    while (*s) kputc(*s++);
}
static void print_uint(unsigned long n, unsigned base) {
    char buf[32];
    int i = 0;
    if (n == 0) { kputc('0'); return; }
    while (n) {
        unsigned d = n % base;
        buf[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        n /= base;
    }
    while (i--) kputc(buf[i]);
}

void printk(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    for (; *fmt; fmt++) {
        if (*fmt != '%') { kputc(*fmt); continue; }
        fmt++;
        switch (*fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            kwrite(s ? s : "(null)");
            break;
        }
        case 'd': {
            int v = va_arg(ap, int);
            if (v < 0) { kputc('-'); v = -v; }
            print_uint((unsigned)v, 10);
            break;
        }
        case 'u': print_uint(va_arg(ap, unsigned), 10); break;
        case 'x': kwrite("0x"); print_uint(va_arg(ap, unsigned), 16); break;
        case 'c': kputc((char)va_arg(ap, int)); break;
        case '%': kputc('%'); break;
        default: kputc('%'); kputc(*fmt); break;
        }
    }
    va_end(ap);
}
