#include "shell.h"
#include "keyboard.h"
#include "printk.h"
#include "task.h"
#include "pmm.h"
#include "timer.h"
#include "vga.h"
#include "syscall.h"
#include <stddef.h>

#define LINE_MAX 128

static void cmd_help(void) {
    printk("Comandos:\n");
    printk("  help  ps  mem  ticks  yield  demo  clear  echo\n");
}

static void worker_a(void) {
    for (int i = 0; i < 5; i++) {
        printk("[A] trabajo %d\n", i);
        for (volatile int d = 0; d < 500000; d++) {}
        task_yield();
    }
    printk("[A] fin\n");
    task_exit();
}

static void worker_b(void) {
    for (int i = 0; i < 5; i++) {
        printk("[B] trabajo %d\n", i);
        for (volatile int d = 0; d < 500000; d++) {}
        task_yield();
    }
    printk("[B] fin\n");
    task_exit();
}

static void run_line(char *line) {
    while (*line == ' ') line++;
    if (!*line) return;
    if (line[0]=='h' && line[1]=='e') { cmd_help(); return; }
    if (line[0]=='p' && line[1]=='s') { task_list(); return; }
    if (line[0]=='m' && line[1]=='e') {
        printk("frames free=%u used=%u total=%u\n",
               pmm_free_frames(), pmm_used_frames(), pmm_total_frames());
        return;
    }
    if (line[0]=='t' && line[1]=='i') { printk("ticks=%u\n", timer_ticks()); return; }
    if (line[0]=='y' && line[1]=='i') { task_yield(); return; }
    if (line[0]=='d' && line[1]=='e') {
        task_create("workerA", worker_a);
        task_create("workerB", worker_b);
        printk("demo workers creados\n");
        return;
    }
    if (line[0]=='c' && line[1]=='l') { vga_clear(); return; }
    if (line[0]=='e' && line[1]=='c') {
        char *p = line + 4;
        while (*p==' ') p++;
        printk("%s\n", p);
        return;
    }
    printk("comando desconocido: %s\n", line);
}

void shell_run(void) {
    char buf[LINE_MAX];
    int pos = 0;
    printk("\nSIF shell — escribí 'help'\n> ");
    for (;;) {
        if (task_needs_resched()) { task_clear_resched(); task_yield(); }
        if (!keyboard_has_input()) { __asm__ volatile ("hlt"); continue; }
        int ch = keyboard_read_char();
        if (!ch) continue;
        if (ch == '\n' || ch == '\r') {
            buf[pos] = 0;
            printk("\n");
            run_line(buf);
            pos = 0;
            printk("> ");
            continue;
        }
        if (ch == '\b') {
            if (pos > 0) { pos--; vga_putc('\b'); }
            continue;
        }
        if (pos < LINE_MAX - 1 && ch >= 32) {
            buf[pos++] = (char)ch;
            vga_putc((char)ch);
        }
    }
}

void shell_task(void) { shell_run(); }
