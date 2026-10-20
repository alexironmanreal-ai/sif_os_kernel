#include "smoke.h"
#include "multiboot.h"
#include "io.h"
#include "printk.h"

void smoke_exit(uint32_t code) {
    /* QEMU isa-debug-exit: device exits with (value >> 1) */
    uint16_t v = (uint16_t)((code << 1) | 1);
    outw(0x501, v);
    for (;;)
        __asm__ volatile("cli; hlt");
}

int smoke_wanted(void) {
    const char *c = multiboot_cmdline();
    if (!c || !c[0])
        return 0;
    for (const char *p = c; *p; p++) {
        if ((p[0] == 's' || p[0] == 'S') &&
            (p[1] == 'm' || p[1] == 'M') &&
            (p[2] == 'o' || p[2] == 'O') &&
            (p[3] == 'k' || p[3] == 'K') &&
            (p[4] == 'e' || p[4] == 'E'))
            return 1;
    }
    return 0;
}

void smoke_note(const char *msg) {
    printk("[smoke] %s\n", msg);
}
