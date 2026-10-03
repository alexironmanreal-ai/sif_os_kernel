#include "syscall.h"
#include "idt.h"
#include "printk.h"
#include "task.h"
#include "timer.h"
#include "rtc.h"
#include "vfs.h"
uint32_t syscall_dispatch(uint32_t num, uint32_t a, uint32_t b, uint32_t c) {
    switch (num) {
    case SYS_EXIT:
        printk("\n[sys] EXIT %u\n", a);
        userland_do_exit();
        return 0;
    case SYS_WRITE:
        if (c > 0x10000) c = 0x10000;
        return (uint32_t)vfs_write((int)a, (const void *)b, c);
    case SYS_YIELD: task_yield(); return 0;
    case SYS_GETPID: {
        struct task *t = task_current();
        return t ? (uint32_t)t->id : (uint32_t)-1;
    }
    case SYS_SLEEP: {
        uint32_t ticks = a / 10; if (!ticks) ticks = 1;
        uint32_t start = timer_ticks();
        while (timer_ticks() - start < ticks) {
            __asm__ volatile ("hlt");
            if (task_needs_resched()) { task_clear_resched(); task_yield(); }
        }
        return 0;
    }
    case SYS_TIME: {
        struct rtc_time t; rtc_read(&t);
        return (uint32_t)t.second + t.minute * 60u + t.hour * 3600u;
    }
    case SYS_OPEN: return (uint32_t)vfs_open((const char *)a, (int)b);
    case SYS_READ:
        if (c > 0x10000) c = 0x10000;
        return (uint32_t)vfs_read((int)a, (void *)b, c);
    case SYS_CLOSE: return (uint32_t)vfs_close((int)a);
    default: printk("[sys] ? %u\n", num); return (uint32_t)-1;
    }
}
void syscall_init(void) {
    extern void isr_syscall(void);
    idt_set_gate(0x80, (uint32_t)isr_syscall, 0x08, 0xEE);
    printk("[sys] int 0x80 + stdio fds\n");
}
void sys_write(const char *s) {
    if (!s) return;
    uint32_t n=0; while(s[n]) n++;
    vfs_write(1, s, n);
}
void sys_yield(void) { __asm__ volatile ("int $0x80" : : "a"(SYS_YIELD) : "memory"); }
int sys_getpid(void) { struct task *t=task_current(); return t?t->id:-1; }
void sys_sleep(uint32_t ms) { __asm__ volatile ("int $0x80" : : "a"(SYS_SLEEP), "b"(ms) : "memory"); }
uint32_t sys_time(void) { return timer_seconds(); }
