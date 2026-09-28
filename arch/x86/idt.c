#include "idt.h"
#include "printk.h"

static struct idt_entry idt[256];
static struct idt_ptr   idtp;

extern void idt_load(uint32_t);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel       = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
}

void idt_init(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint32_t)&idt;
    for (int i = 0; i < 256; i++)
        idt_set_gate(i, 0, 0x08, 0);
    idt_load((uint32_t)&idtp);
    printk("[idt] tabla de interrupciones cargada (%d entradas)\n", 256);
}
