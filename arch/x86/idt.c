#include "idt.h"
#include "printk.h"

static struct idt_entry idt[256];
static struct idt_ptr idtp;

extern void idt_load(uint32_t);
extern void isr_spurious(void);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = (uint16_t)(base & 0xFFFF);
    idt[num].base_high = (uint16_t)((base >> 16) & 0xFFFF);
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_init(void) {
    idtp.limit = (uint16_t)(sizeof(idt) - 1);
    idtp.base = (uint32_t)&idt;

    /*
     * Fill ALL 256 entries — never leave an empty (NULL) gate.
     * Spurious handler absorbs unexpected vectors safely.
     */
    for (int i = 0; i < 256; i++)
        idt_set_gate((uint8_t)i, (uint32_t)isr_spurious, 0x08, 0x8E);

    idt_load((uint32_t)&idtp);
    printk("[idt] 256 gates (default spurious)\n");
}
