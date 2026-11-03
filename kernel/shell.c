#include "shell.h"
#include "cmd.h"
#include "keyboard.h"
#include "serial.h"
#include "printk.h"
#include "task.h"
#include "pmm.h"
#include "vga.h"
#include "ns.h"
#include "dev.h"
#include "net.h"
#include "fs.h"
#include "diskfs.h"
#include "kmalloc.h"
#include "kernel.h"
#include "cpu.h"
#include "io.h"
#include "userland.h"
#include "security.h"
#include "klog.h"
#include "timer.h"
#include "rtc.h"
#include "pci.h"
#include "string.h"
#include <stddef.h>

#define LINE_MAX 160
#define HIST_MAX 16

static char hist[HIST_MAX][LINE_MAX];
static int hist_n;

static void hist_add(const char *line) {
    if (!line || !line[0])
        return;
    if (hist_n > 0 && strcmp(hist[(hist_n - 1) % HIST_MAX], line) == 0)
        return;
    int i = hist_n % HIST_MAX;
    int n = 0;
    while (line[n] && n < LINE_MAX - 1) {
        hist[i][n] = line[n];
        n++;
    }
    hist[i][n] = 0;
    hist_n++;
}

static int console_getchar(void) {
    keyboard_poll();
    if (keyboard_has_input())
        return keyboard_read_char();
    if (serial_has_input()) {
        int c = serial_getc();
        if (c == '\r')
            c = '\n';
        return c;
    }
    return 0;
}

static int c_help(int a, char **v) {
    (void)a;
    (void)v;
    cmd_list();
    return 0;
}

static int c_aid(int a, char **v) {
    (void)a;
    (void)v;
    printk("help | ls cat write cp | mkfs dls | spawn kill\n");
    printk("net ping | date info sec dmesg | hist !!\n");
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

static int c_date(int a, char **v) {
    (void)a;
    (void)v;
    rtc_print();
    return 0;
}

static int c_info(int a, char **v) {
    (void)a;
    (void)v;
    uint32_t used = 0, freeb = 0, blocks = 0;
    kmalloc_stats(&used, &freeb, &blocks);
    printk("%s %s\n", SIF_KERNEL_NAME, SIF_KERNEL_VERSION);
    printk("uptime %u s | frames free %u\n", timer_seconds(),
           pmm_free_frames());
    printk("heap used %u free %u blocks %u\n", used, freeb, blocks);
    printk("disk %s\n", diskfs_mounted() ? "mounted" : "not mounted");
    return 0;
}

static int c_ls(int a, char **v) {
    (void)a;
    (void)v;
    fs_list();
    return 0;
}

static int c_cat(int a, char **v) {
    if (a < 2)
        return -1;
    char buf[1024];
    uint32_t n = 0;
    if (fs_read(v[1], buf, sizeof(buf) - 1, &n) < 0) {
        printk("no such file\n");
        return -1;
    }
    buf[n] = 0;
    printk("%s", buf);
    if (n && buf[n - 1] != '\n')
        printk("\n");
    return 0;
}

static int c_touch(int a, char **v) {
    if (a < 2)
        return -1;
    return fs_create(v[1], "", 0);
}

static int c_rm(int a, char **v) {
    if (a < 2)
        return -1;
    return fs_delete(v[1]);
}

static int c_write(int a, char **v) {
    if (a < 3)
        return -1;
    char body[256];
    int pos = 0;
    for (int i = 2; i < a && pos < 250; i++) {
        if (i > 2)
            body[pos++] = ' ';
        for (char *p = v[i]; *p && pos < 250; p++)
            body[pos++] = *p;
    }
    body[pos++] = '\n';
    return fs_write(v[1], body, (uint32_t)pos);
}

static int c_cp(int a, char **v) {
    if (a < 3) {
        printk("usage: cp src dst\n");
        return -1;
    }
    char buf[4096];
    uint32_t n = 0;
    if (fs_read(v[1], buf, sizeof(buf), &n) < 0) {
        printk("src missing\n");
        return -1;
    }
    if (fs_write(v[2], buf, n) < 0)
        return -1;
    printk("copied %u\n", n);
    return 0;
}

static int c_mkfs(int a, char **v) {
    (void)a;
    (void)v;
    return diskfs_format();
}

static int c_mount(int a, char **v) {
    (void)a;
    (void)v;
    if (diskfs_mount() < 0) {
        printk("mount fail — mkfs?\n");
        return -1;
    }
    diskfs_import_ramfs();
    printk("mounted\n");
    return 0;
}

static int c_sync(int a, char **v) {
    (void)a;
    (void)v;
    return diskfs_sync();
}

static int c_dls(int a, char **v) {
    (void)a;
    (void)v;
    return diskfs_list();
}

static int c_dcat(int a, char **v) {
    if (a < 2)
        return -1;
    char buf[1024];
    uint32_t n = 0;
    if (diskfs_read(v[1], buf, sizeof(buf) - 1, &n) < 0) {
        printk("no disk file\n");
        return -1;
    }
    buf[n] = 0;
    printk("%s", buf);
    if (n && buf[n - 1] != '\n')
        printk("\n");
    return 0;
}

static int c_dwrite(int a, char **v) {
    if (a < 3)
        return -1;
    if (!diskfs_mounted()) {
        printk("not mounted\n");
        return -1;
    }
    char body[256];
    int pos = 0;
    for (int i = 2; i < a && pos < 250; i++) {
        if (i > 2)
            body[pos++] = ' ';
        for (char *p = v[i]; *p && pos < 250; p++)
            body[pos++] = *p;
    }
    body[pos++] = '\n';
    return diskfs_write(v[1], body, (uint32_t)pos);
}

static int c_dmesg(int a, char **v) {
    (void)a;
    (void)v;
    klog_dump(50);
    return 0;
}

static int c_uptime(int a, char **v) {
    (void)a;
    (void)v;
    printk("up %u s (%u ticks)\n", timer_seconds(), timer_ticks());
    return 0;
}

static int c_sec(int a, char **v) {
    (void)a;
    (void)v;
    security_audit();
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
    int ok = 0;
    for (uint16_t s = 1; s <= 3; s++)
        if (icmp_ping(ip, s) == 0)
            ok++;
    printk("%d/3\n", ok);
    return ok ? 0 : -1;
}

static int c_dns(int a, char **v) {
    if (a < 2)
        return -1;
    uint8_t ip[4];
    return dns_resolve(v[1], ip);
}

static int c_http(int a, char **v) {
    const char *host = (a >= 2) ? v[1] : "10.0.2.2";
    const char *path = (a >= 3) ? v[2] : "/";
    static char buf[1024];
    int n = http_get(host, path, buf, sizeof(buf));
    if (n <= 0)
        return -1;
    int show = n > 400 ? 400 : n;
    for (int i = 0; i < show; i++) {
        char c = buf[i];
        if (c >= 32 && c < 127)
            printk("%c", c);
        else if (c == '\n')
            printk("\n");
    }
    printk("\n");
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
        key = v[1];
    char *av[2] = {(char *)key, 0};
    int pid = userland_spawn_file_argv(key, 1, av);
    printk(pid < 0 ? "fail\n" : "id=%d\n", pid);
    return pid < 0 ? -1 : 0;
}

static int c_kill(int a, char **v) {
    if (a < 2)
        return -1;
    int pid = 0;
    for (char *p = v[1]; *p >= '0' && *p <= '9'; p++)
        pid = pid * 10 + (*p - '0');
    if (pid <= 0) {
        printk("bad pid\n");
        return -1;
    }
    if (task_kill(pid) < 0) {
        printk("kill fail\n");
        return -1;
    }
    printk("killed %d\n", pid);
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
    uint32_t used = 0, freeb = 0, blocks = 0;
    kmalloc_stats(&used, &freeb, &blocks);
    printk("frames free=%u\n", pmm_free_frames());
    printk("heap used=%u free=%u blocks=%u\n", used, freeb, blocks);
    return 0;
}

static int c_devs(int a, char **v) {
    (void)a;
    (void)v;
    dev_list();
    return 0;
}

static int c_pci(int a, char **v) {
    (void)a;
    (void)v;
    pci_list();
    return 0;
}

static int c_hist(int a, char **v) {
    (void)a;
    (void)v;
    int start = hist_n > HIST_MAX ? hist_n - HIST_MAX : 0;
    for (int i = start; i < hist_n; i++)
        printk("%d  %s\n", i + 1, hist[i % HIST_MAX]);
    return 0;
}

static int c_clear(int a, char **v) {
    (void)a;
    (void)v;
    vga_clear();
    printk("\033[2J\033[H");
    return 0;
}

static int c_version(int a, char **v) {
    (void)a;
    (void)v;
    printk("%s %s\n", SIF_KERNEL_NAME, SIF_KERNEL_VERSION);
    return 0;
}

static int c_cpu(int a, char **v) {
    (void)a;
    (void)v;
    cpu_report();
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
    cmd_register("help", "list cmds", c_help);
    cmd_register("aid", "shortcuts", c_aid);
    cmd_register("echo", "print", c_echo);
    cmd_register("date", "rtc", c_date);
    cmd_register("info", "sysinfo", c_info);
    cmd_register("ls", "ramfs", c_ls);
    cmd_register("cat", "read", c_cat);
    cmd_register("touch", "create", c_touch);
    cmd_register("rm", "delete", c_rm);
    cmd_register("write", "write", c_write);
    cmd_register("cp", "copy", c_cp);
    cmd_register("mkfs", "format", c_mkfs);
    cmd_register("mount", "mount", c_mount);
    cmd_register("sync", "sync", c_sync);
    cmd_register("dls", "diskls", c_dls);
    cmd_register("dcat", "diskcat", c_dcat);
    cmd_register("dwrite", "diskw", c_dwrite);
    cmd_register("dmesg", "klog", c_dmesg);
    cmd_register("uptime", "up", c_uptime);
    cmd_register("sec", "audit", c_sec);
    cmd_register("net", "iface", c_net);
    cmd_register("arp", "arp", c_arp);
    cmd_register("ping", "icmp", c_ping);
    cmd_register("dns", "dns", c_dns);
    cmd_register("http", "http", c_http);
    cmd_register("cells", "ns", c_cells);
    cmd_register("spawn", "exec", c_spawn);
    cmd_register("kill", "kill", c_kill);
    cmd_register("pulse", "ps", c_pulse);
    cmd_register("vault", "mem", c_vault);
    cmd_register("devs", "dev", c_devs);
    cmd_register("pci", "pci", c_pci);
    cmd_register("hist", "history", c_hist);
    cmd_register("clear", "cls", c_clear);
    cmd_register("version", "ver", c_version);
    cmd_register("cpu", "cpu", c_cpu);
    cmd_register("reboot", "rst", c_reboot);
}

static void run_expanded(char *line) {
    /* !! = last, !n = hist entry n (1-based) */
    if (line[0] == '!' && line[1] == '!' && line[2] == 0) {
        if (hist_n == 0) {
            printk("no history\n");
            return;
        }
        char *h = hist[(hist_n - 1) % HIST_MAX];
        printk("%s\n", h);
        char tmp[LINE_MAX];
        int i = 0;
        while (h[i] && i < LINE_MAX - 1) {
            tmp[i] = h[i];
            i++;
        }
        tmp[i] = 0;
        hist_add(tmp);
        cmd_run_line(tmp);
        return;
    }
    if (line[0] == '!' && line[1] >= '1' && line[1] <= '9') {
        int n = 0;
        for (char *p = line + 1; *p >= '0' && *p <= '9'; p++)
            n = n * 10 + (*p - '0');
        if (n < 1 || n > hist_n) {
            printk("bad !n\n");
            return;
        }
        int start = hist_n > HIST_MAX ? hist_n - HIST_MAX : 0;
        if (n - 1 < start) {
            printk("expired\n");
            return;
        }
        char *h = hist[(n - 1) % HIST_MAX];
        printk("%s\n", h);
        char tmp[LINE_MAX];
        int i = 0;
        while (h[i] && i < LINE_MAX - 1) {
            tmp[i] = h[i];
            i++;
        }
        tmp[i] = 0;
        hist_add(tmp);
        cmd_run_line(tmp);
        return;
    }
    hist_add(line);
    cmd_run_line(line);
}

void shell_run(void) {
    reg();
    char buf[LINE_MAX];
    int pos = 0;
    printk("SIF 3.0 — help | info | hist\nSIF> ");
    for (;;) {
        keyboard_poll();
        net_poll();
        if (task_needs_resched()) {
            task_clear_resched();
            task_yield();
        }
        int ch = console_getchar();
        if (!ch) {
            __asm__ volatile("sti; hlt");
            continue;
        }
        if (ch == '\n' || ch == '\r') {
            buf[pos] = 0;
            printk("\n");
            if (pos > 0)
                run_expanded(buf);
            pos = 0;
            printk("SIF> ");
            continue;
        }
        if (ch == 0x7F || ch == '\b') {
            if (pos > 0) {
                pos--;
                printk("\b \b");
            }
            continue;
        }
        if (pos < LINE_MAX - 1 && ch >= 32 && ch < 127) {
            buf[pos++] = (char)ch;
            printk("%c", ch);
        }
    }
}

void shell_task(void) { shell_run(); }
