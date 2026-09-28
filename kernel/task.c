#include "task.h"
#include "kmalloc.h"
#include "printk.h"
#include <stddef.h>

extern void context_switch(struct cpu_context *old, struct cpu_context *new);

static struct task tasks[MAX_TASKS];
static int current_id = -1;
static int task_count = 0;
static volatile int need_resched = 0;
static uint32_t quantum_left = 10;
#define QUANTUM 10

static void idle_task(void) {
    for (;;) {
        if (need_resched) { need_resched = 0; task_yield(); }
        __asm__ volatile ("hlt");
    }
}

void task_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_UNUSED;
        tasks[i].id = i;
        tasks[i].stack = 0;
        tasks[i].name[0] = 0;
        tasks[i].ticks_ran = 0;
    }
    current_id = -1; task_count = 0; need_resched = 0; quantum_left = QUANTUM;
    task_create("idle", idle_task);
    printk("[task] scheduler listo\n");
}

int task_create(const char *name, void (*entry)(void)) {
    int id = -1;
    for (int i = 0; i < MAX_TASKS; i++)
        if (tasks[i].state == TASK_UNUSED) { id = i; break; }
    if (id < 0) return -1;
    struct task *t = &tasks[id];
    t->stack = (uint32_t *)kmalloc_a(TASK_STACK_SIZE);
    if (!t->stack) return -1;
    t->stack_top = ((uint32_t)t->stack + TASK_STACK_SIZE) & ~0xFu;
    t->ctx.edi = t->ctx.esi = t->ctx.ebp = 0;
    t->ctx.ebx = t->ctx.edx = t->ctx.ecx = t->ctx.eax = 0;
    t->ctx.esp = t->stack_top - 4;
    *((uint32_t *)t->ctx.esp) = (uint32_t)task_exit;
    t->ctx.eip = (uint32_t)entry;
    t->state = TASK_READY;
    t->ticks_ran = 0;
    int n = 0;
    while (name && name[n] && n < 15) { t->name[n] = name[n]; n++; }
    t->name[n] = 0;
    task_count++;
    printk("[task] #%d '%s' eip=%x\n", id, t->name, t->ctx.eip);
    return id;
}

struct task *task_current(void) {
    return (current_id >= 0) ? &tasks[current_id] : 0;
}

void task_exit(void) {
    __asm__ volatile ("cli");
    if (current_id >= 0) {
        tasks[current_id].state = TASK_DEAD;
        printk("[task] exit #%d '%s'\n", current_id, tasks[current_id].name);
    }
    __asm__ volatile ("sti");
    schedule();
    for (;;) __asm__ volatile ("hlt");
}

void schedule(void) {
    int next = -1;
    int start = (current_id + 1 + MAX_TASKS) % MAX_TASKS;
    if (current_id < 0) start = 0;
    for (int n = 0; n < MAX_TASKS; n++) {
        int i = (start + n) % MAX_TASKS;
        if (tasks[i].state == TASK_READY) { next = i; break; }
    }
    if (next < 0) {
        if (current_id >= 0 && tasks[current_id].state == TASK_RUNNING) return;
        for (int i = 0; i < MAX_TASKS; i++)
            if (tasks[i].state == TASK_READY || tasks[i].state == TASK_RUNNING) { next = i; break; }
    }
    if (next < 0) return;
    struct cpu_context *old_ctx;
    static struct cpu_context boot_ctx;
    if (current_id < 0) old_ctx = &boot_ctx;
    else {
        if (tasks[current_id].state == TASK_RUNNING) tasks[current_id].state = TASK_READY;
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
    if (current_id >= 0) tasks[current_id].ticks_ran++;
    if (--quantum_left == 0) { quantum_left = QUANTUM; need_resched = 1; }
}

int task_needs_resched(void) { return need_resched; }
void task_clear_resched(void) { need_resched = 0; }

void task_list(void) {
    printk("ID STATE    TICKS NAME\n");
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED) continue;
        const char *st = "?";
        switch (tasks[i].state) {
        case TASK_READY: st = "READY  "; break;
        case TASK_RUNNING: st = "RUNNING"; break;
        case TASK_BLOCKED: st = "BLOCKED"; break;
        case TASK_DEAD: st = "DEAD   "; break;
        default: break;
        }
        printk("%2d %s %5u %s\n", i, st, tasks[i].ticks_ran, tasks[i].name);
    }
}
