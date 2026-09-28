#ifndef SIF_SYSCALL_H
#define SIF_SYSCALL_H
#include <stdint.h>
#define SYS_EXIT 1
#define SYS_WRITE 2
#define SYS_YIELD 3
#define SYS_GETPID 4
void syscall_init(void);
void sys_write(const char *s);
void sys_yield(void);
int sys_getpid(void);
#endif
