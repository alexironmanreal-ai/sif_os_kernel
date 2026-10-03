#ifndef SIF_SYSCALL_H
#define SIF_SYSCALL_H
#include <stdint.h>
#define SYS_EXIT 1
#define SYS_WRITE 2
#define SYS_YIELD 3
#define SYS_GETPID 4
#define SYS_SLEEP 5
#define SYS_TIME 6
#define SYS_READ 7
#define SYS_OPEN 8
void syscall_init(void);
void sys_write(const char *s);
void sys_yield(void);
int sys_getpid(void);
void sys_sleep(uint32_t ms);
uint32_t sys_time(void);
#endif
