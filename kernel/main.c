#include "kernel.h"
#include "vga.h"
#include "printk.h"
#include "gdt.h"
#include "idt.h"

void panic(const char *msg) {
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    printk("\n*** KERNEL PANIC: %s ***\n", msg);
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

void kernel_main(uint32_t magic, void *mbi) {
    (void)mbi;

    vga_init();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("%s v%s\n", SIF_KERNEL_NAME, SIF_KERNEL_VERSION);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("Base de kernel x86 (32-bit) — Multiboot\n");
    printk("----------------------------------------\n");

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printk("[boot] magic=%x (esperado %x)\n", magic, MULTIBOOT_BOOTLOADER_MAGIC);
        printk("[boot] aviso: no iniciado por Multiboot compatible\n");
    } else {
        printk("[boot] Multiboot OK (magic=%x)\n", magic);
    }

    printk("[cpu] iniciando GDT...\n");
    gdt_init();
    printk("[cpu] GDT lista (null/code/data ring0)\n");

    printk("[cpu] iniciando IDT...\n");
    idt_init();

    printk("----------------------------------------\n");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printk("Kernel base operativo.\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("Proximo paso: memoria, paging, scheduler, syscalls.\n");
    printk("Construí el SO arriba de esta base.\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
