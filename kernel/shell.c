#include "shell.h"
#include "cmd.h"
#include "keyboard.h"
#include "printk.h"
#include "task.h"
#include "pmm.h"
#include "timer.h"
#include "vga.h"
#include "ns.h"
#include "dev.h"
#include "wire.h"
#include "net.h"
#include "rtc.h"
#include "kmalloc.h"
#include "kernel.h"
#include "pci.h"
#include "multiboot.h"
#include "cpu.h"
#include "io.h"
#include "userland.h"
#include "gui.h"
#include "bios.h"
#include "ata.h"
#include <stddef.h>

#define LINE_MAX 160

static int parse_int(const char *s) {
    int n = 0, neg = 0;
    if (*s == '-') {
        neg = 1;
        s++;
    }
    while (*s >= '0' && *s <= '9')
        n = n * 10 + (*s++ - '0');
    return neg ? -n : n;
}

static int c_aid(int a, char **v) {
    (void)a;
    (void)v;
    printk("net arp ping | cells spawn | cpu lm boot\n");
    return 0;
}

static int c_net(int a, char **v) {
    (void)a;
    (void)v;
    net_poll();
    net_status();
    return 0;
}

static int c_arp(int a, char **v) {
    if (a >= 2) {
        uint8_t ip[4];
        if (net_parse_ip(v[1], ip) < 0) {
            printk("usage: arp [a.b.c.d]\n");
            return -1;
        }
        printk("arp who-has %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
        arp_request(ip);
        for (int t = 0; t < 10; t++) {
            for (volatile int i = 0; i < 80000; i++)
                ;
            net_poll();
        }
    }
    arp_dump();
    return 0;
}

static int c_ping(int a, char **v) {
    if (a < 2) {
        printk("usage: ping a.b.c.d\n");
        return -1;
    }
    uint8_t ip[4];
    if (net_parse_ip(v[1], ip) < 0) {
        printk("bad ip\n");
        return -1;
    }
    printk("PING %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
    int ok = 0;
    for (uint16_t seq = 1; seq <= 3; seq++) {
        if (icmp_ping(ip, seq) == 0)
            ok++;
    }
    printk("%d/3 replies\n", ok);
    return ok ? 0 : -1;
}

static int c_boot(int a, char **v) {
    (void)a;
    (void)v;
    multiboot_dump();
    return 0;
}

static int c_cpu(int a, char **v) {
    (void)a;
    (void)v;
    cpu_report();
    return 0;
}

static int c_lm(int a, char **v) {
    (void)a;
    (void)v;
    if (!cpu_has_long_mode()) {
        printk("no LM\n");
        return -1;
    }
    printk("entering long mode...\n");
    longmode_enter();
    return 0;
}

static int c_cells(int a, char **v) {
    ns_cells((a >= 2) ? v[1] : "");
    return 0;
}

static int c_glance(int a, char **v) {
    if (a < 2)
        return -1;
    char b[512];
    uint32_t sz = 0;
    if (ns_glance(v[1], b, sizeof(b) - 1, &sz) < 0)
        return -1;
    b[sz] = 0;
    printk("%s", b);
    if (sz && b[sz - 1] != '\n')
        printk("\n");
    return 0;
}

static int c_spawn(int a, char **v) {
    if (a < 2)
        return -1;
    const char *key = ns_store_key(v[1]);
    if (!key)
        return -1;
    char *argv_local[8];
    int argc = a - 1;
    if (argc > 7)
        argc = 7;
    argv_local[0] = (char *)key;
    for (int i = 1; i < argc; i++)
        argv_local[i] = v[i + 1];
    int pid = userland_spawn_file_argv(key, argc, argv_local);
    if (pid < 0)
        return -1;
    printk("spawned id=%d\n", pid);
    return 0;
}

static int c_pulse(int a, char **v) {
    (void)a;
    (void)v;
    task_list();
    return 0;
}

static int c_vault(int a, char **v) {
    (void)a;
    (void)v;
    uint32_t u, f, b;
    kmalloc_stats(&u, &f, &b);
    printk("free frames=%u heap used=%u\n", pmm_free_frames(), u);
    return 0;
}

static int c_devs(int a, char **v) {
    (void)a;
    (void)v;
    dev_list();
    return 0;
}

static int c_desk(int a, char **v) {
    (void)a;
    (void)v;
    gui_run();
    return 0;
}

static int c_clear(int a, char **v) {
    (void)a;
    (void)v;
    vga_clear();
    return 0;
}

static int c_echo(int a, char **v) {
    for (int i = 1; i < a; i++) {
        if (i > 1)
            printk(" ");
        printk("%s", v[i]);
    }
    printk("\n");
    return 0;
}

static int c_version(int a, char **v) {
    (void)a;
    (void)v;
    printk("%s %s\n", SIF_KERNEL_NAME, SIF_KERNEL_VERSION);
    return 0;
}

static int c_reboot(int a, char **v) {
    (void)a;
    (void)v;
    uint8_t g = 0x02;
    while (g & 0x02)
        g = inb(0x64);
    outb(0x64, 0xFE);
    for (;;)
        __asm__ volatile("hlt");
    return 0;
}

static void reg(void) {
    cmd_init();
    cmd_register("aid", "help", c_aid);
    cmd_register("net", "iface", c_net);
    cmd_register("arp", "arp", c_arp);
    cmd_register("ping", "icmp", c_ping);
    cmd_register("boot", "multiboot", c_boot);
    cmd_register("cpu", "cpu", c_cpu);
    cmd_register("lm", "longmode", c_lm);
    cmd_register("cells", "ns", c_cells);
    cmd_register("glance", "read", c_glance);
    cmd_register("spawn", "exec", c_spawn);
    cmd_register("pulse", "ps", c_pulse);
    cmd_register("vault", "mem", c_vault);
    cmd_register("devs", "devices", c_devs);
    cmd_register("desk", "gui", c_desk);
    cmd_register("clear", "cls", c_clear);
    cmd_register("echo", "print", c_echo);
    cmd_register("version", "ver", c_version);
    cmd_register("reboot", "reset", c_reboot);
}

void shell_run(void) {
    reg();
    char buf[LINE_MAX];
    int pos = 0;
    printk("SIF 2.5 net — net | arp | ping 10.0.2.2\nSIF> ");
    for (;;) {
        net_poll();
        if (task_needs_resched()) {
            task_clear_resched();
            task_yield();
        }
        if (!keyboard_has_input()) {
            __asm__ volatile("hlt");
            continue;
        }
        int ch = keyboard_read_char();
        if (!ch)
            continue;
        if (ch == '\n' || ch == '\r') {
            buf[pos] = 0;
            printk("\n");
            if (pos > 0)
                cmd_run_line(buf);
            pos = 0;
            printk("SIF> ");
            continue;
        }
        if (ch == '\b') {
            if (pos > 0) {
                pos--;
                vga_putc('\b');
            }
            continue;
        }
        if (pos < LINE_MAX - 1 && ch >= 32 && ch < 127) {
            buf[pos++] = (char)ch;
            vga_putc((char)ch);
        }
    }
}

void shell_task(void) { shell_run(); }
