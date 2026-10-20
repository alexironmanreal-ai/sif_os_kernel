#include "shell.h"
#include "cmd.h"
#include "keyboard.h"
#include "printk.h"
#include "task.h"
#include "pmm.h"
#include "vga.h"
#include "ns.h"
#include "dev.h"
#include "net.h"
#include "kmalloc.h"
#include "kernel.h"
#include "multiboot.h"
#include "cpu.h"
#include "io.h"
#include "userland.h"
#include "gui.h"
#include <stddef.h>

#define LINE_MAX 160

static int c_aid(int a, char **v) {
    (void)a;
    (void)v;
    printk("net arp ping dns http | cells spawn | cpu\n");
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
        if (net_parse_ip(v[1], ip) == 0) {
            arp_request(ip);
            for (int t = 0; t < 10; t++) {
                for (volatile int i = 0; i < 80000; i++)
                    ;
                net_poll();
            }
        }
    }
    arp_dump();
    return 0;
}

static int c_ping(int a, char **v) {
    if (a < 2)
        return -1;
    uint8_t ip[4];
    if (net_parse_ip(v[1], ip) < 0)
        return -1;
    printk("PING %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
    int ok = 0;
    for (uint16_t s = 1; s <= 3; s++)
        if (icmp_ping(ip, s) == 0)
            ok++;
    printk("%d/3\n", ok);
    return ok ? 0 : -1;
}

static int c_dns(int a, char **v) {
    if (a < 2) {
        printk("usage: dns name\n");
        return -1;
    }
    uint8_t ip[4];
    return dns_resolve(v[1], ip);
}

static int c_http(int a, char **v) {
    const char *host = (a >= 2) ? v[1] : "10.0.2.2";
    const char *path = (a >= 3) ? v[2] : "/";
    static char buf[1024];
    int n = http_get(host, path, buf, sizeof(buf));
    if (n <= 0) {
        printk("http fail\n");
        return -1;
    }
    /* print first lines */
    int show = n > 400 ? 400 : n;
    for (int i = 0; i < show; i++) {
        char c = buf[i];
        if (c >= 32 && c < 127)
            printk("%c", c);
        else if (c == '\n')
            printk("\n");
        else if (c == '\r')
            ;
        else
            printk(".");
    }
    printk("\n");
    return 0;
}

static int c_udp(int a, char **v) {
    if (a < 3) {
        printk("usage: udp a.b.c.d text\n");
        return -1;
    }
    uint8_t ip[4];
    if (net_parse_ip(v[1], ip) < 0)
        return -1;
    char body[128];
    int pos = 0;
    for (int i = 2; i < a && pos < 120; i++) {
        if (i > 2)
            body[pos++] = ' ';
        for (char *p = v[i]; *p && pos < 120; p++)
            body[pos++] = *p;
    }
    body[pos] = 0;
    int r = udp_send(ip, 4000, 7, body, (uint16_t)pos); /* echo port 7 */
    printk(r < 0 ? "udp fail\n" : "udp %d B\n", r);
    return r < 0 ? -1 : 0;
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

static int c_cells(int a, char **v) {
    ns_cells((a >= 2) ? v[1] : "");
    return 0;
}

static int c_spawn(int a, char **v) {
    if (a < 2)
        return -1;
    const char *key = ns_store_key(v[1]);
    if (!key)
        return -1;
    char *av[4];
    av[0] = (char *)key;
    int pid = userland_spawn_file_argv(key, 1, av);
    printk(pid < 0 ? "fail\n" : "id=%d\n", pid);
    return pid < 0 ? -1 : 0;
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
    printk("free frames=%u\n", pmm_free_frames());
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
    cmd_register("dns", "resolve", c_dns);
    cmd_register("http", "get", c_http);
    cmd_register("udp", "udp", c_udp);
    cmd_register("boot", "mb", c_boot);
    cmd_register("cpu", "cpu", c_cpu);
    cmd_register("cells", "ns", c_cells);
    cmd_register("spawn", "exec", c_spawn);
    cmd_register("pulse", "ps", c_pulse);
    cmd_register("vault", "mem", c_vault);
    cmd_register("devs", "dev", c_devs);
    cmd_register("desk", "gui", c_desk);
    cmd_register("clear", "cls", c_clear);
    cmd_register("version", "ver", c_version);
    cmd_register("reboot", "rst", c_reboot);
}

void shell_run(void) {
    reg();
    char buf[LINE_MAX];
    int pos = 0;
    printk("SIF 2.6 — net arp ping dns http\nSIF> ");
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
