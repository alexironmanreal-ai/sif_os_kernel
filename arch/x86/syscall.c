#include "syscall.h"
#include "idt.h"
#include "printk.h"
#include "task.h"

void syscall_dispatch(uint32_t num, uint32_t a, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    switch (num) {
    case SYS_EXIT: task_exit(); break;
    case SYS_WRITE: if (a) printk("%s", (const char *)a); break;
    case SYS_YIELD: task_yield(); break;
    case SYS_GETPID: {
        struct task *t = task_current();
        printk("%d", t ? t->id : -1);
        break;
    }
    default: printk("[sys] ? %u\n", num); break;
    }
}

void syscall_init(void) {
    extern void isr_syscall(void);
    idt_set_gate(0x80, (uint32_t)isr_syscall, 0x08, 0x8E);
    printk("[sys] int 0x80 listo\n");
}

void sys_write(const char *s) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_WRITE), "b"(s) : "memory");
}
void sys_yield(void) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_YIELD) : "memory");
}
int sys_getpid(void) {
    struct task *t = task_current();
    return t ? t->id : -1;
}
