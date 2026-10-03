#include "stdio.h"
#include "unistd.h"

int putchar(int c) {
    char ch = (char)c;
    write(STDOUT_FILENO, &ch, 1);
    return c;
}

int puts(const char *s) {
    unsigned n = 0;
    while (s[n]) n++;
    write(STDOUT_FILENO, s, n);
    write(STDOUT_FILENO, "\n", 1);
    return 0;
}

static void print_uint(unsigned v) {
    char buf[16];
    int i = 0;
    if (!v) { putchar('0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i--) putchar(buf[i]);
}

static void print_hex(unsigned v) {
    const char *h = "0123456789abcdef";
    putchar('0');
    putchar('x');
    for (int i = 7; i >= 0; i--)
        putchar(h[(v >> (i * 4)) & 0xF]);
}

int printf(const char *fmt, ...) {
    unsigned *args = (unsigned *)((char *)&fmt + sizeof(fmt));
    int ai = 0;
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') { putchar(*p); continue; }
        p++;
        if (!*p) break;
        switch (*p) {
        case 's': {
            const char *s = (const char *)args[ai++];
            if (!s) s = "(null)";
            while (*s) putchar(*s++);
            break;
        }
        case 'd': {
            int v = (int)args[ai++];
            if (v < 0) { putchar('-'); v = -v; }
            print_uint((unsigned)v);
            break;
        }
        case 'u':
            print_uint(args[ai++]);
            break;
        case 'x':
            print_hex(args[ai++]);
            break;
        case 'c':
            putchar((int)args[ai++]);
            break;
        case '%':
            putchar('%');
            break;
        default:
            putchar('%');
            putchar(*p);
            break;
        }
    }
    return 0;
}

int getchar(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) <= 0) return -1;
    return (unsigned char)c;
}

int fgets(char *buf, int max) {
    int i = 0;
    while (i < max - 1) {
        int c = getchar();
        if (c < 0) break;
        buf[i++] = (char)c;
        if (c == '\n' || c == '\r') break;
    }
    buf[i] = 0;
    return i;
}
