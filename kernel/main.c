#include "kernel.h"
#include "vga.h"
#include "printk.h"
#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "pic.h"
#include "serial.h"
#include "timer.h"
#include "keyboard.h"

void panic(const char *msg) {
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    printk("\n*** KERNEL PANIC: %s ***\n", msg);
    for (;;) { __asm__ volatile ("cli; hlt"); }
}

void kernel_main(uint32_t magic, void *mbi) {
    (void)mbi;
    vga_init();
    serial_init();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("%s v%s\n", SIF_KERNEL_NAME, SIF_KERNEL_VERSION);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("Base x86 Multiboot — Fase 1 (PIC/PIT/KBD/Serial)\n");
    printk("========================================\n");
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        printk("[boot] magic=%x (esperado %x)\n", magic, MULTIBOOT_BOOTLOADER_MAGIC);
    else
        printk("[boot] Multiboot OK\n");
    gdt_init();
    printk("[cpu] GDT lista\n");
    idt_init();
    irq_init();
    timer_init(100);
    keyboard_init();
    __asm__ volatile ("sti");
    printk("[cpu] interrupciones habilitadas (STI)\n");
    printk("========================================\n");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printk("Kernel operativo. Escribí en el teclado.\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("Ticks del timer cada ~5s.\n\n");
    uint32_t last_report = 0;
    for (;;) {
        uint32_t t = timer_ticks();
        if (t - last_report >= 500) {
            printk("[timer] ticks=%u seconds~%u\n", t, timer_seconds());
            last_report = t;
        }
        while (keyboard_has_input()) (void)keyboard_read_char();
        __asm__ volatile ("hlt");
    }
}
