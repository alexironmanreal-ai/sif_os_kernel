#include "syscall.h"
#include "idt.h"
#include "printk.h"
#include "task.h"
#include "timer.h"
#include "rtc.h"
#include "fs.h"
void syscall_dispatch(uint32_t num, uint32_t a, uint32_t b, uint32_t c) {
    (void)c;
    switch (num) {
    case SYS_EXIT:
        printk("[sys] EXIT (userspace fin)\n");
        for (;;) { __asm__ volatile ("sti; hlt"); }
        break;
    case SYS_WRITE:
        if (a) printk("%s", (const char *)a);
        break;
    case SYS_YIELD: task_yield(); break;
    case SYS_GETPID: {
        struct task *t = task_current();
        printk("%d", t ? t->id : -1);
        break;
    }
    case SYS_SLEEP: {
        uint32_t ticks = a / 10; if (!ticks) ticks = 1;
        uint32_t start = timer_ticks();
        while (timer_ticks() - start < ticks) {
            __asm__ volatile ("hlt");
            if (task_needs_resched()) { task_clear_resched(); task_yield(); }
        }
        break;
    }
    case SYS_TIME: {
        struct rtc_time t; rtc_read(&t);
        printk("%u", (unsigned)t.second + t.minute * 60u + t.hour * 3600u);
        break;
    }
    case SYS_OPEN:
        printk(a && fs_exists((const char *)a) ? "1" : "0");
        break;
    case SYS_READ: {
        if (!a) break;
        char tmp[256]; uint32_t sz = 0;
        if (fs_read((const char *)a, tmp, sizeof(tmp) - 1, &sz) == 0) {
            tmp[sz] = 0; printk("%s", tmp);
        }
        (void)b; break;
    }
    default: printk("[sys] ? %u\n", num); break;
    }
}
void syscall_init(void) {
    extern void isr_syscall(void);
    idt_set_gate(0x80, (uint32_t)isr_syscall, 0x08, 0xEE);
    printk("[sys] int 0x80 (DPL3) listo\n");
}
void sys_write(const char *s) { __asm__ volatile ("int $0x80" : : "a"(SYS_WRITE), "b"(s) : "memory"); }
void sys_yield(void) { __asm__ volatile ("int $0x80" : : "a"(SYS_YIELD) : "memory"); }
int sys_getpid(void) { struct task *t = task_current(); return t ? t->id : -1; }
void sys_sleep(uint32_t ms) { __asm__ volatile ("int $0x80" : : "a"(SYS_SLEEP), "b"(ms) : "memory"); }
uint32_t sys_time(void) { return timer_seconds(); }
