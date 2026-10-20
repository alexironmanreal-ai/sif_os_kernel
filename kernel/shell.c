#include "shell.h"
#include "cmd.h"
#include "keyboard.h"
#include "printk.h"
#include "task.h"
#include "pmm.h"
#include "timer.h"
#include "vga.h"
#include "ns.h"
#include "rtc.h"
#include "kmalloc.h"
#include "kernel.h"
#include "pci.h"
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

/* ---- SIF native commands ---- */

static int c_aid(int a, char **v) {
    (void)a;
    (void)v;
    printk("\n=== SIF console ===\n");
    printk("  aid              this help\n");
    printk("  cells [prefix]   list cells (apps / docs)\n");
    printk("  glance PATH      read cell\n");
    printk("  forge PATH       create empty cell\n");
    printk("  scribe PATH TXT  write cell\n");
    printk("  erase PATH       delete cell\n");
    printk("  spawn PATH       launch ELF async\n");
    printk("  await [id]       wait process\n");
    printk("  halt ID          stop process\n");
    printk("  pulse            process list\n");
    printk("  vault            memory\n");
    printk("  desk             graphical desk\n");
    printk("  setup            system setup screen\n");
    printk("  tick / when      uptime / clock\n");
    printk("  clear / echo\n");
    printk("Paths use '.'  e.g. apps.hello  docs.motd\n\n");
    return 0;
}

static int c_cells(int a, char **v) {
    const char *p = (a >= 2) ? v[1] : "";
    ns_cells(p);
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
    if (ns_forge(v[1]) < 0) {
        printk("forge fail\n");
        return -1;
    }
    printk("forged %s\n", v[1]);
    return 0;
}

static int c_scribe(int a, char **v) {
    if (a < 3) {
        printk("scribe PATH text...\n");
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
    if (ns_scribe(v[1], body, (uint32_t)pos) < 0) {
        printk("scribe fail\n");
        return -1;
    }
    printk("ok\n");
    return 0;
}

static int c_erase(int a, char **v) {
    if (a < 2) {
        printk("erase PATH\n");
        return -1;
    }
    if (ns_erase(v[1]) < 0) {
        printk("erase fail\n");
        return -1;
    }
    printk("erased\n");
    return 0;
}

static int c_spawn(int a, char **v) {
    if (a < 2) {
        printk("spawn PATH [args]\n");
        return -1;
    }
    const char *key = ns_store_key(v[1]);
    if (!key) {
        printk("bad path\n");
        return -1;
    }
    /* argv[0] should be store key for loader */
    char *argv_local[8];
    int argc = a - 1;
    if (argc > 7)
        argc = 7;
    argv_local[0] = (char *)key;
    for (int i = 1; i < argc; i++)
        argv_local[i] = v[i + 1];
    int pid = userland_spawn_file_argv(key, argc, argv_local);
    if (pid < 0) {
        printk("spawn fail\n");
        return -1;
    }
    printk("spawned id=%d\n", pid);
    return 0;
}

static int c_await(int a, char **v) {
    int pid = -1;
    if (a >= 2)
        pid = parse_int(v[1]);
    int st = 0;
    int w = task_wait(pid, &st);
    if (w < 0) {
        printk("await fail\n");
        return -1;
    }
    printk("await id=%d code=%d\n", w, st);
    return 0;
}

static int c_halt(int a, char **v) {
    if (a < 2) {
        printk("halt ID\n");
        return -1;
    }
    int pid = parse_int(v[1]);
    if (task_kill(pid) < 0) {
        printk("halt fail\n");
        return -1;
    }
    printk("halted %d\n", pid);
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
    printk("frames free=%u used=%u total=%u\n", pmm_free_frames(),
           pmm_used_frames(), pmm_total_frames());
    printk("heap used=%u free=%u blocks=%u\n", u, f, b);
    return 0;
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
    printk("up %um %us  ticks=%u\n", s / 60, s % 60, timer_ticks());
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
    printk("ata %s  sfs %s  pci %d\n", ata_present() ? "yes" : "no",
           diskfs_mounted() ? "yes" : "no", pci_count());
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
    cmd_register("cells", "list cells", c_cells);
    cmd_register("glance", "read cell", c_glance);
    cmd_register("forge", "create cell", c_forge);
    cmd_register("scribe", "write cell", c_scribe);
    cmd_register("erase", "delete cell", c_erase);
    cmd_register("spawn", "launch ELF", c_spawn);
    cmd_register("await", "wait process", c_await);
    cmd_register("halt", "stop process", c_halt);
    cmd_register("pulse", "processes", c_pulse);
    cmd_register("vault", "memory", c_vault);
    cmd_register("desk", "graphics", c_desk);
    cmd_register("setup", "setup screen", c_setup);
    cmd_register("tick", "uptime", c_tick);
    cmd_register("when", "RTC clock", c_when);
    cmd_register("clear", "clear screen", c_clear);
    cmd_register("echo", "print", c_echo);
    cmd_register("version", "version", c_version);
    cmd_register("probe", "hardware", c_probe);
    cmd_register("reboot", "reset", c_reboot);
}

void shell_run(void) {
    reg();
    char buf[LINE_MAX];
    int pos = 0;
    printk("SIF 2.0 console — aid | cells | spawn | desk\nSIF> ");
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
