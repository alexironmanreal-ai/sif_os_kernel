#include "syscall.h"
#include "idt.h"
#include "printk.h"
#include "task.h"
#include "timer.h"
#include "rtc.h"
#include "vfs.h"
#include "pmm.h"
#include "paging.h"
#include "string.h"
static uint32_t user_brk = 0x08100000u;
static uint32_t user_brk_mapped = 0x08100000u;
uint32_t syscall_dispatch(uint32_t num, uint32_t a, uint32_t b, uint32_t c) {
    switch (num) {
    case SYS_EXIT:
        printk("\n[sys] EXIT %u\n", a);
        { struct task *t = task_current(); if (t) task_exit_code((int)a); else userland_do_exit(); }
        return 0;
    case SYS_WRITE:
        if (c > 0x10000) c = 0x10000;
        return (uint32_t)vfs_write((int)a, (const void *)b, c);
    case SYS_YIELD: task_yield(); return 0;
    case SYS_GETPID: { struct task *t = task_current(); return t ? (uint32_t)t->id : 0; }
    case SYS_SLEEP: task_sleep_ms(a); return 0;
    case SYS_TIME: { struct rtc_time t; rtc_read(&t); return (uint32_t)t.second+t.minute*60u+t.hour*3600u; }
    case SYS_OPEN: return (uint32_t)vfs_open((const char *)a, (int)b);
    case SYS_READ: if (c > 0x10000) c = 0x10000; return (uint32_t)vfs_read((int)a, (void *)b, c);
    case SYS_CLOSE: return (uint32_t)vfs_close((int)a);
    case SYS_KILL: return (uint32_t)task_kill((int)a);
    case SYS_WAIT: { int st=0; int pid=task_wait((int)a,&st); if(b) *(int*)b=st; return (uint32_t)pid; }
    case SYS_BRK:
        if (a==0) return user_brk;
        if (a < 0x08100000u || a > 0x08F00000u) return user_brk;
        while (user_brk_mapped < a) {
            uint32_t frame = pmm_alloc_frame(); if (!frame) break;
            paging_map(user_brk_mapped, frame, PAGE_PRESENT|PAGE_WRITE|PAGE_USER);
            memset((void*)user_brk_mapped, 0, 4096);
            user_brk_mapped += 4096;
        }
        user_brk = a; return user_brk;
    default: printk("[sys] ? %u\n", num); return (uint32_t)-1;
    }
}
void syscall_init(void) {
    extern void isr_syscall(void);
    idt_set_gate(0x80, (uint32_t)isr_syscall, 0x08, 0xEE);
    printk("[sys] int 0x80 + kill/wait/brk\n");
}
void sys_write(const char *s) { if(!s)return; uint32_t n=0; while(s[n])n++; vfs_write(1,s,n); }
void sys_yield(void) { __asm__ volatile("int $0x80"::"a"(SYS_YIELD):"memory"); }
int sys_getpid(void) { struct task *t=task_current(); return t?t->id:0; }
void sys_sleep(uint32_t ms) { task_sleep_ms(ms); }
uint32_t sys_time(void) { return timer_seconds(); }
