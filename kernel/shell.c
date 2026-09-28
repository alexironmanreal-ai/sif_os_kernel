#include "shell.h"
#include "keyboard.h"
#include "printk.h"
#include "task.h"
#include "pmm.h"
#include "timer.h"
#include "vga.h"
#include "fs.h"
#include "ata.h"
#include "kmalloc.h"
#include "string.h"
#include "kernel.h"
#include "io.h"
#include <stddef.h>

#define LINE_MAX 128

static void cmd_help(void) {
    printk("\nComandos disponibles:\n");
    printk("  help          - esta ayuda\n");
    printk("  ps            - listar tareas\n");
    printk("  mem           - memoria fisica (frames)\n");
    printk("  heap          - estadisticas del heap\n");
    printk("  ticks         - ticks del timer\n");
    printk("  yield         - ceder CPU\n");
    printk("  demo          - crear 2 workers de demo\n");
    printk("  clear         - limpiar pantalla\n");
    printk("  echo TEXTO    - imprimir texto\n");
    printk("  ls            - listar archivos (ramfs)\n");
    printk("  cat ARCHIVO   - mostrar archivo\n");
    printk("  write NOMBRE TEXTO - crear/sobreescribir archivo\n");
    printk("  rm ARCHIVO    - borrar archivo\n");
    printk("  disk          - probar lectura ATA LBA0\n");
    printk("  version       - version del kernel\n");
    printk("  reboot        - reiniciar (triple fault)\n\n");
}

static void worker_a(void) {
    for (int i = 0; i < 5; i++) {
        printk("[A] %d\n", i);
        for (volatile int d = 0; d < 800000; d++) {}
        task_yield();
    }
    printk("[A] fin\n");
    task_exit();
}

static void worker_b(void) {
    for (int i = 0; i < 5; i++) {
        printk("[B] %d\n", i);
        for (volatile int d = 0; d < 800000; d++) {}
        task_yield();
    }
    printk("[B] fin\n");
    task_exit();
}

static const char *skip_ws(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static int starts(const char *s, const char *p) {
    while (*p) {
        if (*s++ != *p++) return 0;
    }
    return 1;
}

static void run_line(char *line) {
    line = (char *)skip_ws(line);
    if (!*line) return;

    if (starts(line, "help"))   { cmd_help(); return; }
    if (starts(line, "ps"))     { task_list(); return; }
    if (starts(line, "mem")) {
        printk("frames: free=%u used=%u total=%u (~%u MiB)\n",
               pmm_free_frames(), pmm_used_frames(), pmm_total_frames(),
               (pmm_total_frames() * 4) / 1024);
        return;
    }
    if (starts(line, "heap"))   { kmalloc_stats(); return; }
    if (starts(line, "ticks"))  { printk("ticks=%u\n", timer_ticks()); return; }
    if (starts(line, "yield"))  { task_yield(); return; }
    if (starts(line, "demo")) {
        task_create("workerA", worker_a);
        task_create("workerB", worker_b);
        printk("workers creados\n");
        return;
    }
    if (starts(line, "clear"))  { vga_clear(); return; }
    if (starts(line, "echo")) {
        const char *p = skip_ws(line + 4);
        printk("%s\n", p);
        return;
    }
    if (starts(line, "ls"))     { fs_list(); return; }
    if (starts(line, "cat ")) {
        const char *n = skip_ws(line + 4);
        char buf[512];
        uint32_t sz = 0;
        if (fs_read(n, buf, sizeof(buf) - 1, &sz) < 0) {
            printk("no existe: %s\n", n);
            return;
        }
        buf[sz] = 0;
        printk("%s", buf);
        if (sz && buf[sz - 1] != '\n') printk("\n");
        return;
    }
    if (starts(line, "write ")) {
        char *p = (char *)skip_ws(line + 6);
        char name[FS_NAME_LEN];
        int i = 0;
        while (*p && *p != ' ' && i < FS_NAME_LEN - 1) name[i++] = *p++;
        name[i] = 0;
        p = (char *)skip_ws(p);
        if (!name[0]) { printk("uso: write NOMBRE TEXTO\n"); return; }
        size_t len = strlen(p);
        if (fs_create(name, p, (uint32_t)len) < 0)
            printk("error creando %s\n", name);
        else
            printk("ok %s (%u bytes)\n", name, (uint32_t)len);
        return;
    }
    if (starts(line, "rm ")) {
        const char *n = skip_ws(line + 3);
        if (fs_delete(n) < 0) printk("no existe: %s\n", n);
        else printk("borrado: %s\n", n);
        return;
    }
    if (starts(line, "disk")) {
        if (!ata_present()) printk("ATA: sin disco\n");
        else {
            uint8_t sec[512];
            if (ata_read_sectors(0, 1, sec) == 0)
                printk("LBA0 OK (primeros bytes: %x %x %x %x)\n",
                       sec[0], sec[1], sec[2], sec[3]);
            else
                printk("error leyendo LBA0\n");
        }
        return;
    }
    if (starts(line, "version")) {
        printk("%s v%s\n", SIF_KERNEL_NAME, SIF_KERNEL_VERSION);
        return;
    }
    if (starts(line, "reboot")) {
        printk("reiniciando...\n");
        __asm__ volatile ("cli");
        outb(0x64, 0xFE);
        for (;;) __asm__ volatile ("hlt");
    }

    printk("desconocido: %s  (escribe help)\n", line);
}

void shell_run(void) {
    char buf[LINE_MAX];
    int pos = 0;
    printk("SIF> ");
    for (;;) {
        if (task_needs_resched()) {
            task_clear_resched();
            task_yield();
        }
        if (!keyboard_has_input()) {
            __asm__ volatile ("hlt");
            continue;
        }
        int ch = keyboard_read_char();
        if (!ch) continue;

        if (ch == '\n' || ch == '\r') {
            buf[pos] = 0;
            printk("\n");
            run_line(buf);
            pos = 0;
            printk("SIF> ");
            continue;
        }
        if (ch == '\b' || ch == 127) {
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
