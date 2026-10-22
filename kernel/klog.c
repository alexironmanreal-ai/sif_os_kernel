#include "klog.h"
#include "string.h"

#define KLOG_SIZE 4096

static char buf[KLOG_SIZE];
static uint32_t head; /* next write */
static uint32_t total;
static int inited;

void klog_init(void) {
    head = 0;
    total = 0;
    inited = 1;
    memset(buf, 0, sizeof(buf));
}

void klog_putc(char c) {
    if (!inited)
        klog_init();
    buf[head] = c;
    head = (head + 1) % KLOG_SIZE;
    if (total < KLOG_SIZE)
        total++;
}

void klog_write(const char *s, uint32_t n) {
    if (!s)
        return;
    for (uint32_t i = 0; i < n; i++)
        klog_putc(s[i]);
}

uint32_t klog_bytes(void) { return total; }

/* Forward declare to avoid circular include with printk */
void serial_putc(char c);
void vga_putc(char c);
int gfx_is_graphics(void);

static void outc(char c) {
    if (!gfx_is_graphics())
        vga_putc(c);
    serial_putc(c);
}

void klog_dump(uint32_t max_lines) {
    if (!inited || total == 0) {
        outc('(');
        outc('e');
        outc('m');
        outc('p');
        outc('t');
        outc('y');
        outc(')');
        outc('\n');
        return;
    }
    uint32_t start = (total < KLOG_SIZE) ? 0 : head;
    uint32_t count = total;
    uint32_t lines = 0;
    uint32_t i = 0;
    /* if max_lines, skip older lines */
    if (max_lines > 0) {
        uint32_t seen = 0;
        for (uint32_t j = 0; j < count; j++) {
            char c = buf[(start + j) % KLOG_SIZE];
            if (c == '\n')
                seen++;
        }
        if (seen > max_lines) {
            uint32_t skip = seen - max_lines;
            uint32_t j = 0;
            while (j < count && skip) {
                if (buf[(start + j) % KLOG_SIZE] == '\n')
                    skip--;
                j++;
            }
            i = j;
        }
    }
    for (; i < count; i++) {
        char c = buf[(start + i) % KLOG_SIZE];
        outc(c);
        if (c == '\n')
            lines++;
    }
    if (count && buf[(start + count - 1) % KLOG_SIZE] != '\n')
        outc('\n');
    (void)lines;
}
