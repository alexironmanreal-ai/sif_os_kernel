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
#include "multiboot.h"
#include "pmm.h"
#include "paging.h"
#include "kmalloc.h"

extern uint32_t kernel_end;

void panic(const char *msg) {
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    printk("\n*** KERNEL PANIC: %s ***\n", msg);
    for (;;) { __asm__ volatile ("cli; hlt"); }
}

void kernel_main(uint32_t magic, void *mbi_ptr) {
    struct multiboot_info *mbi = (struct multiboot_info *)mbi_ptr;
    vga_init();
    serial_init();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("%s v%s\n", SIF_KERNEL_NAME, SIF_KERNEL_VERSION);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("Fase 1+2: IRQ/timer/kbd + memoria\n");
    printk("========================================\n");
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printk("[boot] magic=%x (QEMU -kernel puede diferir)\n", magic);
        mbi = 0;
    } else printk("[boot] Multiboot OK\n");
    gdt_init(); printk("[cpu] GDT lista\n");
    idt_init();
    irq_init();
    uint32_t kend = (uint32_t)&kernel_end;
    printk("[mm] kernel_end=%x\n", kend);
    pmm_init(mbi, kend);
    paging_init();
    kmalloc_init();
    void *a = kmalloc(64);
    void *b = kmalloc(128);
    printk("[kmalloc] test a=%x b=%x free_frames=%u\n", (uint32_t)a, (uint32_t)b, pmm_free_frames());
    timer_init(100);
    keyboard_init();
    __asm__ volatile ("sti");
    printk("[cpu] STI \u2014 interrupciones ON\n");
    printk("========================================\n");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printk("Escribi con el teclado. Timer cada 5s.\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("\n");
    uint32_t last_report = 0;
    for (;;) {
        uint32_t t = timer_ticks();
        if (t - last_report >= 500) {
            printk("[timer] ticks=%u free_frames=%u\n", t, pmm_free_frames());
            last_report = t;
        }
        while (keyboard_has_input()) (void)keyboard_read_char();
        __asm__ volatile ("hlt");
    }
}
