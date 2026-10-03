#include "task.h"
#include "kmalloc.h"
#include "printk.h"
#include "timer.h"
#include "string.h"

extern void context_switch(struct cpu_context *old, struct cpu_context *new);

static struct task tasks[MAX_TASKS];
static int current_id = -1;
static int task_count = 0;
static int need_resched = 0;
static int quantum_left = 0;
#define QUANTUM 5

static void idle_task(void) {
    for (;;) {
        __asm__ volatile ("sti; hlt");
        if (need_resched) {
            need_resched = 0;
            schedule();
        }
    }
}

void task_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_UNUSED;
        tasks[i].id = i;
        tasks[i].stack = 0;
        tasks[i].name[0] = 0;
        tasks[i].wake_tick = 0;
        tasks[i].exit_code = 0;
        tasks[i].wait_parent = -1;
        tasks[i].waiting_on = -1;
    }
    current_id = -1;
    quantum_left = QUANTUM;
    task_count = 0;
    task_create("idle", idle_task);
    printk("[task] scheduler listo\n");
}

int task_create(const char *name, void (*entry)(void)) {
    int id = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED || tasks[i].state == TASK_DEAD) {
            id = i; break;
        }
    }
    if (id < 0) return -1;

    struct task *t = &tasks[id];
    if (!t->stack) {
        t->stack = (uint32_t *)kmalloc_a(TASK_STACK_SIZE);
        if (!t->stack) return -1;
    }
    t->stack_top = ((uint32_t)t->stack + TASK_STACK_SIZE) & ~0xFu;
    t->ctx.edi = t->ctx.esi = t->ctx.ebp = 0;
    t->ctx.ebx = t->ctx.edx = t->ctx.ecx = t->ctx.eax = 0;
    t->ctx.esp = t->stack_top - 4;
    *((uint32_t *)t->ctx.esp) = (uint32_t)task_exit;
    t->ctx.eip = (uint32_t)entry;
    t->state = TASK_READY;
    t->ticks_ran = 0;
    t->wake_tick = 0;
    t->exit_code = 0;
    t->wait_parent = -1;
    t->waiting_on = -1;
    int n = 0;
    while (name && name[n] && n < 15) { t->name[n] = name[n]; n++; }
    t->name[n] = 0;
    task_count++;
    printk("[task] #%d '%s'\n", id, t->name);
    return id;
}

struct task *task_current(void) {
    return (current_id >= 0) ? &tasks[current_id] : 0;
}

void task_exit_code(int code) {
    __asm__ volatile ("cli");
    if (current_id >= 0) {
        tasks[current_id].exit_code = code;
        tasks[current_id].state = TASK_ZOMBIE;
        int p = tasks[current_id].wait_parent;
        if (p >= 0 && tasks[p].state == TASK_BLOCKED)
            tasks[p].state = TASK_READY;
        for (int i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].state == TASK_BLOCKED &&
                (tasks[i].waiting_on == current_id || tasks[i].waiting_on == -1))
                tasks[i].state = TASK_READY;
        }
    }
    schedule();
    for (;;) __asm__ volatile ("hlt");
}

void task_exit(void) {
    task_exit_code(0);
}

void task_sleep_ms(uint32_t ms) {
    if (current_id < 0) {
        uint32_t ticks = ms / 10; if (!ticks) ticks = 1;
        uint32_t start = timer_ticks();
        while (timer_ticks() - start < ticks)
            __asm__ volatile ("hlt");
        return;
    }
    uint32_t ticks = ms / 10;
    if (!ticks) ticks = 1;
    tasks[current_id].wake_tick = timer_ticks() + ticks;
    tasks[current_id].state = TASK_SLEEPING;
    schedule();
}

int task_kill(int pid) {
    if (pid < 0 || pid >= MAX_TASKS) return -1;
    if (tasks[pid].state == TASK_UNUSED || tasks[pid].state == TASK_DEAD)
        return -1;
    if (pid == 0) return -1;
    tasks[pid].exit_code = -9;
    tasks[pid].state = TASK_ZOMBIE;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_BLOCKED &&
            (tasks[i].waiting_on == pid || tasks[i].waiting_on == -1))
            tasks[i].state = TASK_READY;
    }
    if (pid == current_id)
        schedule();
    return 0;
}

int task_wait(int pid, int *status) {
    if (current_id < 0) return -1;
    for (;;) {
        int found = -1;
        for (int i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].state != TASK_ZOMBIE) continue;
            if (pid >= 0 && i != pid) continue;
            found = i;
            break;
        }
        if (found >= 0) {
            if (status) *status = tasks[found].exit_code;
            tasks[found].state = TASK_DEAD;
            return found;
        }
        tasks[current_id].state = TASK_BLOCKED;
        tasks[current_id].waiting_on = pid;
        if (pid >= 0 && pid < MAX_TASKS)
            tasks[pid].wait_parent = current_id;
        schedule();
    }
}

int task_get_exit(int pid) {
    if (pid < 0 || pid >= MAX_TASKS) return -1;
    if (tasks[pid].state == TASK_ZOMBIE || tasks[pid].state == TASK_DEAD)
        return tasks[pid].exit_code;
    return -1;
}

void schedule(void) {
    uint32_t now = timer_ticks();
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_SLEEPING && now >= tasks[i].wake_tick)
            tasks[i].state = TASK_READY;
    }

    int next = -1;
    int start = (current_id < 0) ? 0 : (current_id + 1);
    for (int n = 0; n < MAX_TASKS; n++) {
        int i = (start + n) % MAX_TASKS;
        if (tasks[i].state == TASK_READY) { next = i; break; }
    }
    if (next < 0) {
        if (current_id >= 0 &&
            (tasks[current_id].state == TASK_RUNNING ||
             tasks[current_id].state == TASK_READY))
            return;
        for (int i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].state == TASK_READY || tasks[i].state == TASK_RUNNING) {
                next = i; break;
            }
        }
    }
    if (next < 0) return;

    static struct cpu_context boot_ctx;
    struct cpu_context *old_ctx;
    if (current_id < 0) {
        old_ctx = &boot_ctx;
    } else {
        if (tasks[current_id].state == TASK_RUNNING)
            tasks[current_id].state = TASK_READY;
        old_ctx = &tasks[current_id].ctx;
    }

    tasks[next].state = TASK_RUNNING;
    int prev = current_id;
    current_id = next;
    quantum_left = QUANTUM;
    if (prev == next) return;
    context_switch(old_ctx, &tasks[next].ctx);
}

void task_yield(void) { schedule(); }

void task_on_timer(void) {
    uint32_t now = timer_ticks();
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_SLEEPING && now >= tasks[i].wake_tick) {
            tasks[i].state = TASK_READY;
            need_resched = 1;
        }
    }
    if (current_id >= 0)
        tasks[current_id].ticks_ran++;
    if (--quantum_left <= 0) {
        quantum_left = QUANTUM;
        need_resched = 1;
    }
}

int task_needs_resched(void) { return need_resched; }
void task_clear_resched(void) { need_resched = 0; }

void task_list(void) {
    printk("ID STATE     TICKS NAME\n");
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED) continue;
        const char *st = "?";
        switch (tasks[i].state) {
        case TASK_READY:    st = "READY   "; break;
        case TASK_RUNNING:  st = "RUNNING "; break;
        case TASK_BLOCKED:  st = "BLOCKED "; break;
        case TASK_SLEEPING: st = "SLEEPING"; break;
        case TASK_ZOMBIE:   st = "ZOMBIE  "; break;
        case TASK_DEAD:     st = "DEAD    "; break;
        default: break;
        }
        printk("%2d %s %5u %s\n", i, st, tasks[i].ticks_ran, tasks[i].name);
    }
}
