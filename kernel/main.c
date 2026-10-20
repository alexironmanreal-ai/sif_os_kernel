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
#include "mouse.h"
#include "multiboot.h"
#include "pmm.h"
#include "paging.h"
#include "kmalloc.h"
#include "task.h"
#include "syscall.h"
#include "shell.h"
#include "gui.h"
#include "ata.h"
#include "fs.h"
#include "ns.h"
#include "dev.h"
#include "wire.h"
#include "diskfs.h"
#include "rtc.h"
#include "pci.h"
#include "userland.h"
#include "vfs.h"
#include "cpu.h"
#include "smoke.h"

void panic(const char *msg) {
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    printk("\n*** KERNEL PANIC: %s ***\n", msg);
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

void kernel_main(uint32_t magic, void *mbi_ptr) {
    struct multiboot_info *mbi = (struct multiboot_info *)mbi_ptr;

    vga_init();
    serial_init();
    multiboot_save(magic, mbi);

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("========================================\n");
    printk("  %s v%s\n", SIF_KERNEL_NAME, SIF_KERNEL_VERSION);
    printk("========================================\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    printk("[boot] higher-half VMA=%x\n", KERNEL_VMA);
    printk("[boot] kernel %x - %x (phys %x - %x)\n",
           (uint32_t)&kernel_start, (uint32_t)&kernel_end,
           (uint32_t)&kernel_phys_start, (uint32_t)&kernel_phys_end);

    if (multiboot_ok()) {
        printk("[boot] %s\n", multiboot_loader_name());
        if (!multiboot_is_v2() && multiboot_cmdline()[0])
            printk("[boot] cmdline: %s\n", multiboot_cmdline());
    } else {
        printk("[boot] bad magic=%x\n", magic);
        mbi = 0;
    }

    if (multiboot_is_v2())
        mbi = 0;

    gdt_init();
    idt_init();
    isr_install();
    irq_init();
    syscall_init();
    pmm_init(mbi, (uint32_t)&kernel_end);
    paging_init();
    kmalloc_init();
    timer_init(100);
    keyboard_init();
    task_init();
    ata_init();
    fs_init();
    ns_init();
    diskfs_init();
    vfs_init();
    rtc_init();
    pci_init();
    userland_init();

    __asm__ volatile("sti");
    printk("[ok] interrupts armed\n");
    mouse_init();
    dev_init();
    wire_init();

    if (smoke_wanted()) {
        smoke_note("PASS higher-half");
        printk("[smoke] PASS\n");
        smoke_exit(0);
    }

    printk("----------------------------------------\n");
    printk("  HH kernel | idt256 | wire | aid\n");
    printk("----------------------------------------\n");
    gui_run();

    if (task_create("console", shell_task) < 0)
        panic("no console");

    for (;;) {
        schedule();
        __asm__ volatile("sti; hlt");
    }
}
