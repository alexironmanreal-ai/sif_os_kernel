#ifndef SIF_TASK_H
#define SIF_TASK_H
#include <stdint.h>
#include <stddef.h>

#define MAX_TASKS 16
#define TASK_STACK_SIZE 4096
#define TASK_KSTACK_SIZE 8192
#define PROC_MAX_FD 16
#define PROC_PATH_LEN 32

enum task_state {
    TASK_UNUSED = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_ZOMBIE,
    TASK_DEAD
};

struct cpu_context {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax, eip;
};

/* per-process file descriptor */
struct proc_fd {
    int used;
    int kind; /* 1=special, 2=file */
    int special_id;
    int writable;
    char name[PROC_PATH_LEN];
    uint32_t offset;
};

struct task {
    int id;
    enum task_state state;
    struct cpu_context ctx;
    uint32_t *stack;
    uint32_t stack_top;
    char name[16];
    uint32_t ticks_ran;
    uint32_t wake_tick;
    int exit_code;
    int wait_parent;
    int waiting_on;

    /* process / address-space */
    uint32_t cr3;
    int is_user;
    int parent;
    uint32_t *kstack;
    uint32_t kstack_top;
    uint32_t user_eip;
    uint32_t user_esp;

    /* per-process resources */
    struct proc_fd fds[PROC_MAX_FD];
    uint32_t brk;
    uint32_t brk_mapped;
};

void task_init(void);
int task_create(const char *name, void (*entry)(void));
int task_create_user(const char *name, uint32_t cr3, uint32_t entry,
                     uint32_t user_esp);
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
void task_switch_address_space(struct task *t);
void task_init_stdio(struct task *t);
void task_close_all_fds(struct task *t);

#endif
