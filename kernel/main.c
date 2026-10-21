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
#include "ns.h"
#include "dev.h"
#include "wire.h"
#include "net.h"
#include "diskfs.h"
#include "rtc.h"
#include "pci.h"
#include "userland.h"
#include "vfs.h"
#include "cpu.h"
#include "smoke.h"
#include "security.h"

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
    security_init();
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
    /* mouse_init skipped in console mode — was stealing kbd bytes */
    dev_init();
    net_init();
    wire_init();

    if (smoke_wanted()) {
        smoke_note("PASS console");
        printk("[smoke] PASS\n");
        smoke_exit(0);
    }

    printk("----------------------------------------\n");
    printk("  console — type at SIF> prompt\n");
    printk("----------------------------------------\n");

    if (task_create("console", shell_task) < 0)
        panic("no console");

    for (;;) {
        keyboard_poll();
        net_poll();
        schedule();
        __asm__ volatile("sti; hlt");
    }
}
