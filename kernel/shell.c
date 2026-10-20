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
#include "rtc.h"
#include "kmalloc.h"
#include "kernel.h"
#include "pci.h"
#include "multiboot.h"
#include "syscall.h"
#include "io.h"
#include "userland.h"
#include "gui.h"
#include "bios.h"
#include "ata.h"
#include "diskfs.h"
#include <stddef.h>

#define LINE_MAX 160
#define HIST_MAX 16

static char hist[HIST_MAX][LINE_MAX];
static int hist_count;

static void hist_add(const char *line) {
    if (!line[0])
        return;
    int i = 0;
    while (line[i] && i < LINE_MAX - 1) {
        hist[hist_count % HIST_MAX][i] = line[i];
        i++;
    }
    hist[hist_count % HIST_MAX][i] = 0;
    hist_count++;
}

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
    printk("\n=== SIF console ===\n");
    printk("  boot   Multiboot / memory map\n");
    printk("  wire cast sniff\n");
    printk("  cells spawn desk\n\n");
    return 0;
}

static int c_boot(int a, char **v) {
    (void)a;
    (void)v;
    multiboot_dump();
    return 0;
}

static int c_cells(int a, char **v) {
    ns_cells((a >= 2) ? v[1] : "");
    return 0;
}

static int c_glance(int a, char **v) {
    if (a < 2) {
        printk("glance PATH\n");
        return -1;
    }
    char b[512];
    uint32_t sz = 0;
    if (ns_glance(v[1], b, sizeof(b) - 1, &sz) < 0) {
        printk("no cell\n");
        return -1;
    }
    b[sz] = 0;
    printk("%s", b);
    if (sz && b[sz - 1] != '\n')
        printk("\n");
    return 0;
}

static int c_forge(int a, char **v) {
    if (a < 2) {
        printk("forge PATH\n");
        return -1;
    }
    return ns_forge(v[1]) < 0 ? -1 : 0;
}

static int c_scribe(int a, char **v) {
    if (a < 3) {
        printk("scribe PATH text\n");
        return -1;
    }
    char body[256];
    int pos = 0;
    for (int i = 2; i < a && pos < 255; i++) {
        if (i > 2)
            body[pos++] = ' ';
        for (char *p = v[i]; *p && pos < 255; p++)
            body[pos++] = *p;
    }
    body[pos] = 0;
    return ns_scribe(v[1], body, (uint32_t)pos) < 0 ? -1 : 0;
}

static int c_erase(int a, char **v) {
    if (a < 2)
        return -1;
    return ns_erase(v[1]);
}

static int c_link(int a, char **v) {
    if (a < 3)
        return -1;
    return ns_link(v[1], v[2]);
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

static int c_await(int a, char **v) {
    int pid = -1;
    if (a >= 2)
        pid = parse_int(v[1]);
    int st = 0;
    int w = task_wait(pid, &st);
    if (w < 0)
        return -1;
    printk("await id=%d code=%d\n", w, st);
    return 0;
}

static int c_halt(int a, char **v) {
    if (a < 2)
        return -1;
    return task_kill(parse_int(v[1]));
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
    printk("frames free=%u used=%u\n", pmm_free_frames(), pmm_used_frames());
    printk("heap used=%u free=%u\n", u, f);
    return 0;
}

static int c_devs(int a, char **v) {
    (void)a;
    (void)v;
    dev_list();
    return 0;
}

static int c_wire(int a, char **v) {
    (void)a;
    (void)v;
    wire_status();
    return 0;
}

static int c_cast(int a, char **v) {
    if (a < 2)
        return -1;
    char body[200];
    int pos = 0;
    for (int i = 1; i < a && pos < 199; i++) {
        if (i > 1)
            body[pos++] = ' ';
        for (char *p = v[i]; *p && pos < 199; p++)
            body[pos++] = *p;
    }
    body[pos] = 0;
    int r = wire_cast_text(body);
    printk(r < 0 ? "cast fail\n" : "cast %d B\n", r);
    return r < 0 ? -1 : 0;
}

static int c_sniff(int a, char **v) {
    (void)a;
    (void)v;
    int n = wire_sniff(1);
    if (n == 0)
        printk("(no frame)\n");
    return n < 0 ? -1 : 0;
}

static int c_desk(int a, char **v) {
    (void)a;
    (void)v;
    gui_run();
    return 0;
}

static int c_setup(int a, char **v) {
    (void)a;
    (void)v;
    bios_run();
    return 0;
}

static int c_tick(int a, char **v) {
    (void)a;
    (void)v;
    uint32_t s = timer_seconds();
    printk("up %um %us\n", s / 60, s % 60);
    return 0;
}

static int c_when(int a, char **v) {
    (void)a;
    (void)v;
    rtc_print();
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

static int c_probe(int a, char **v) {
    (void)a;
    (void)v;
    printk("ata %s  pci %d  wire %s  mb %s\n", ata_present() ? "yes" : "no",
           pci_count(), wire_up() ? "UP" : "DOWN",
           multiboot_ok() ? "OK" : "no");
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
    cmd_register("boot", "multiboot", c_boot);
    cmd_register("cells", "list", c_cells);
    cmd_register("glance", "read", c_glance);
    cmd_register("forge", "create", c_forge);
    cmd_register("scribe", "write", c_scribe);
    cmd_register("erase", "delete", c_erase);
    cmd_register("link", "bind", c_link);
    cmd_register("spawn", "launch", c_spawn);
    cmd_register("await", "wait", c_await);
    cmd_register("halt", "kill", c_halt);
    cmd_register("pulse", "ps", c_pulse);
    cmd_register("vault", "mem", c_vault);
    cmd_register("devs", "devices", c_devs);
    cmd_register("wire", "net", c_wire);
    cmd_register("cast", "bcast", c_cast);
    cmd_register("sniff", "recv", c_sniff);
    cmd_register("desk", "gui", c_desk);
    cmd_register("setup", "setup", c_setup);
    cmd_register("tick", "uptime", c_tick);
    cmd_register("when", "clock", c_when);
    cmd_register("clear", "cls", c_clear);
    cmd_register("echo", "print", c_echo);
    cmd_register("version", "ver", c_version);
    cmd_register("probe", "hw", c_probe);
    cmd_register("reboot", "reset", c_reboot);
}

void shell_run(void) {
    reg();
    char buf[LINE_MAX];
    int pos = 0;
    printk("SIF 2.2.1 — boot | wire | aid\nSIF> ");
    for (;;) {
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
            if (pos > 0) {
                hist_add(buf);
                char w[LINE_MAX];
                int i = 0;
                while (buf[i] && i < LINE_MAX - 1) {
                    w[i] = buf[i];
                    i++;
                }
                w[i] = 0;
                cmd_run_line(w);
            }
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
