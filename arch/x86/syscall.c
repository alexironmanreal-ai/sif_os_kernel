#include "syscall.h"
#include "idt.h"
#include "printk.h"
#include "task.h"
#include "timer.h"
#include "rtc.h"
#include "vfs.h"
#include "fs.h"
#include "pmm.h"
#include "paging.h"
#include "string.h"
#include "userland.h"

#define USER_BRK_START 0x08100000u
#define USER_BRK_MAX   0x08F00000u

uint32_t syscall_dispatch(uint32_t num, uint32_t a, uint32_t b, uint32_t c) {
    switch (num) {
    case SYS_EXIT:
        printk("\n[sys] EXIT %u\n", a);
        {
            struct task *t = task_current();
            if (t)
                task_exit_code((int)a);
            else
                userland_do_exit();
        }
        return 0;

    case SYS_WRITE:
        if (c > 0x10000)
            c = 0x10000;
        return (uint32_t)vfs_write((int)a, (const void *)b, c);

    case SYS_YIELD:
        task_yield();
        return 0;

    case SYS_GETPID: {
        struct task *t = task_current();
        return t ? (uint32_t)t->id : 0;
    }

    case SYS_SLEEP:
        task_sleep_ms(a);
        return 0;

    case SYS_TIME: {
        struct rtc_time t;
        rtc_read(&t);
        return (uint32_t)t.second + t.minute * 60u + t.hour * 3600u;
    }

    case SYS_OPEN:
        return (uint32_t)vfs_open((const char *)a, (int)b);

    case SYS_READ:
        if (c > 0x10000)
            c = 0x10000;
        return (uint32_t)vfs_read((int)a, (void *)b, c);

    case SYS_CLOSE:
        return (uint32_t)vfs_close((int)a);

    case SYS_KILL:
        return (uint32_t)task_kill((int)a);

    case SYS_WAIT: {
        int st = 0;
        int pid = task_wait((int)a, &st);
        if (b)
            *(int *)b = st;
        return (uint32_t)pid;
    }

    case SYS_BRK: {
        struct task *t = task_current();
        if (!t || !t->is_user) {
            static uint32_t g_brk = USER_BRK_START;
            static uint32_t g_mapped = USER_BRK_START;
            if (a == 0)
                return g_brk;
            if (a < USER_BRK_START || a > USER_BRK_MAX)
                return g_brk;
            while (g_mapped < a) {
                uint32_t frame = pmm_alloc_frame();
                if (!frame)
                    break;
                paging_map(g_mapped, frame,
                           PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
                memset((void *)g_mapped, 0, 4096);
                g_mapped += 4096;
            }
            g_brk = a;
            return g_brk;
        }
        if (a == 0)
            return t->brk;
        if (a < USER_BRK_START || a > USER_BRK_MAX)
            return t->brk;
        while (t->brk_mapped < a) {
            uint32_t frame = pmm_alloc_frame();
            if (!frame)
                break;
            paging_map(t->brk_mapped, frame,
                       PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
            memset((void *)t->brk_mapped, 0, 4096);
            t->brk_mapped += 4096;
        }
        t->brk = a;
        return t->brk;
    }

    case SYS_GETDENTS:
        if (!a || b == 0 || b > 0x10000)
            return (uint32_t)-1;
        return (uint32_t)fs_getdents((char *)a, b);

    default:
        printk("[sys] ? %u\n", num);
        return (uint32_t)-1;
    }
}

void syscall_init(void) {
    extern void isr_syscall(void);
    idt_set_gate(0x80, (uint32_t)isr_syscall, 0x08, 0xEE);
    printk("[sys] int 0x80 async-ready\n");
}

void sys_write(const char *s) {
    if (!s)
        return;
    uint32_t n = 0;
    while (s[n])
        n++;
    vfs_write(1, s, n);
}

void sys_yield(void) {
    __asm__ volatile("int $0x80" ::"a"(SYS_YIELD) : "memory");
}

int sys_getpid(void) {
    struct task *t = task_current();
    return t ? t->id : 0;
}

void sys_sleep(uint32_t ms) { task_sleep_ms(ms); }

uint32_t sys_time(void) { return timer_seconds(); }
