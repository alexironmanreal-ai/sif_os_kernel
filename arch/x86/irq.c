#include "irq.h"
#include "idt.h"
#include "pic.h"
#include "printk.h"

extern void irq0(void);  extern void irq1(void);
extern void irq2(void);  extern void irq3(void);
extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);
extern void irq8(void);  extern void irq9(void);
extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void);
extern void irq14(void); extern void irq15(void);

static irq_handler_t handlers[16];

void irq_dispatch(uint8_t irq) {
    if (irq < 16 && handlers[irq])
        handlers[irq]();
    pic_send_eoi(irq);
}

void irq_install_handler(uint8_t irq, irq_handler_t handler) {
    if (irq < 16) {
        handlers[irq] = handler;
        pic_clear_mask(irq);
    }
}

void irq_uninstall_handler(uint8_t irq) {
    if (irq < 16) {
        handlers[irq] = 0;
        pic_set_mask(irq);
    }
}

void irq_init(void) {
    for (int i = 0; i < 16; i++)
        handlers[i] = 0;
    pic_mask_all();
    pic_remap(0x20, 0x28);
    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3,  0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4,  0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5,  0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6,  0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7,  0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
    printk("[irq] PIC remapeado (IRQ0-15 -> int 0x20-0x2F)\n");
}
