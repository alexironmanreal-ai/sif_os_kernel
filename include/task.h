#ifndef SIF_TASK_H
#define SIF_TASK_H
#include <stdint.h>
#include <stddef.h>
#define MAX_TASKS 16
#define TASK_STACK_SIZE 4096
enum task_state { TASK_UNUSED=0, TASK_READY, TASK_RUNNING, TASK_BLOCKED, TASK_SLEEPING, TASK_ZOMBIE, TASK_DEAD };
struct cpu_context { uint32_t edi,esi,ebp,esp,ebx,edx,ecx,eax,eip; };
struct task {
  int id; enum task_state state; struct cpu_context ctx;
  uint32_t *stack; uint32_t stack_top; char name[16]; uint32_t ticks_ran;
  uint32_t wake_tick; int exit_code; int wait_parent; int waiting_on;
};
void task_init(void);
int task_create(const char *name, void (*entry)(void));
void task_yield(void);
void task_exit(void);
void task_exit_code(int code);
void schedule(void);
struct task *task_current(void);
void task_on_timer(void);
int task_needs_resched(void);
void task_clear_resched(void);
void task_list(void);
void task_sleep_ms(uint32_t ms);
int task_kill(int pid);
int task_wait(int pid, int *status);
int task_get_exit(int pid);
#endif
