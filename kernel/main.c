#include "kernel.h"
#include "vga.h"
#include "printk.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "serial.h"
#include "timer.h"
#include "keyboard.h"
#include "multiboot.h"
#include "pmm.h"
#include "paging.h"
#include "kmalloc.h"
#include "task.h"
#include "syscall.h"
#include "shell.h"
#include "ata.h"
#include "fs.h"
#include "rtc.h"
#include "pci.h"
#include "userland.h"
#include "vfs.h"
extern uint32_t kernel_end;
void panic(const char *msg) {
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    printk("\n*** KERNEL PANIC: %s ***\n", msg);
    for (;;) { __asm__ volatile ("cli; hlt"); }
}
void kernel_main(uint32_t magic, void *mbi_ptr) {
    struct multiboot_info *mbi = (struct multiboot_info *)mbi_ptr;
    vga_init(); serial_init();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("========================================\n");
    printk("  %s v%s\n", SIF_KERNEL_NAME, SIF_KERNEL_VERSION);
    printk("========================================\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) { printk("[boot] magic=%x\n", magic); mbi = 0; }
    else printk("[boot] Multiboot OK\n");
    gdt_init(); idt_init(); isr_install(); irq_init(); syscall_init();
    pmm_init(mbi, (uint32_t)&kernel_end);
    paging_init(); kmalloc_init();
    timer_init(100); keyboard_init(); task_init();
    ata_init(); fs_init(); vfs_init(); rtc_init(); pci_init(); userland_init();
    __asm__ volatile ("sti");
    printk("[ok] interrupciones ON\n");
    printk("----------------------------------------\n");
    printk("help | user | exec hello.elf | open | ls\n");
    printk("----------------------------------------\n\n");
    shell_run();
    panic("shell returned");
}
