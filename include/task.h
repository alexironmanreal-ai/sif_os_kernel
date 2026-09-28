#ifndef SIF_TASK_H
#define SIF_TASK_H
#include <stdint.h>
#include <stddef.h>
#define MAX_TASKS 16
#define TASK_STACK_SIZE 4096
enum task_state { TASK_UNUSED=0, TASK_READY, TASK_RUNNING, TASK_BLOCKED, TASK_DEAD };
struct cpu_context { uint32_t edi,esi,ebp,esp,ebx,edx,ecx,eax; uint32_t eip; };
struct task { int id; enum task_state state; struct cpu_context ctx; uint32_t *stack; uint32_t stack_top; char name[16]; uint32_t ticks_ran; };
void task_init(void);
int task_create(const char *name, void (*entry)(void));
void task_yield(void);
void task_exit(void);
void schedule(void);
struct task *task_current(void);
void task_on_timer(void);
int task_needs_resched(void);
void task_clear_resched(void);
void task_list(void);
#endif
