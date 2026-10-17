#include "task.h"
#include "kmalloc.h"
#include "printk.h"
#include "timer.h"
#include "string.h"
#include "paging.h"
#include "gdt.h"

extern void context_switch(struct cpu_context *old, struct cpu_context *new);
extern void enter_usermode(uint32_t eip, uint32_t esp);

static struct task tasks[MAX_TASKS];
static int current_id = -1;
static int task_count = 0;
static int need_resched = 0;
static int quantum_left = 0;
#define QUANTUM 5

#define USER_BRK_START 0x08100000u

/* Forward: kernel stub that enters ring3 for user tasks */
static void user_trampoline(void);

static void idle_task(void) {
    for (;;) {
        __asm__ volatile("sti; hlt");
        if (need_resched) {
            need_resched = 0;
            schedule();
        }
    }
}

static void user_trampoline(void) {
    struct task *t = task_current();
    if (!t || !t->is_user || !t->user_eip) {
        printk("[task] bad user trampoline\n");
        task_exit_code(-1);
    }
    if (t->kstack_top)
        tss_set_kernel_stack(t->kstack_top);
    if (t->cr3)
        paging_switch(t->cr3);
    enter_usermode(t->user_eip, t->user_esp);
    task_exit_code(-1);
}

void task_init_stdio(struct task *t) {
    if (!t)
        return;
    for (int i = 0; i < PROC_MAX_FD; i++) {
        t->fds[i].used = 0;
        t->fds[i].name[0] = 0;
        t->fds[i].offset = 0;
        t->fds[i].kind = 0;
        t->fds[i].special_id = 0;
        t->fds[i].writable = 0;
    }
    t->fds[0].used = 1;
    t->fds[0].kind = 1;
    t->fds[0].special_id = 0;
    t->fds[0].writable = 0;
    t->fds[1].used = 1;
    t->fds[1].kind = 1;
    t->fds[1].special_id = 1;
    t->fds[1].writable = 1;
    t->fds[2].used = 1;
    t->fds[2].kind = 1;
    t->fds[2].special_id = 2;
    t->fds[2].writable = 1;
    t->brk = USER_BRK_START;
    t->brk_mapped = USER_BRK_START;
}

void task_close_all_fds(struct task *t) {
    if (!t)
        return;
    for (int i = 3; i < PROC_MAX_FD; i++) {
        t->fds[i].used = 0;
        t->fds[i].name[0] = 0;
        t->fds[i].offset = 0;
    }
}

void task_set_ready(int id) {
    if (id >= 0 && id < MAX_TASKS &&
        (tasks[id].state == TASK_BLOCKED || tasks[id].state == TASK_SLEEPING))
        tasks[id].state = TASK_READY;
}

void task_switch_address_space(struct task *t) {
    uint32_t want = (t && t->cr3) ? t->cr3 : paging_kernel_pd();
    if (want != paging_current_pd())
        paging_switch(want);
    if (t && t->kstack_top)
        tss_set_kernel_stack(t->kstack_top);
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
        tasks[i].cr3 = 0;
        tasks[i].is_user = 0;
        tasks[i].parent = -1;
        tasks[i].kstack = 0;
        tasks[i].kstack_top = 0;
        tasks[i].user_eip = 0;
        tasks[i].user_esp = 0;
        task_init_stdio(&tasks[i]);
    }
    current_id = -1;
    quantum_left = QUANTUM;
    task_count = 0;
    task_create("idle", idle_task);
    printk("[task] scheduler + async user tasks\n");
}

int task_create(const char *name, void (*entry)(void)) {
    int id = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED || tasks[i].state == TASK_DEAD) {
            id = i;
            break;
        }
    }
    if (id < 0)
        return -1;

    struct task *t = &tasks[id];
    if (!t->stack) {
        t->stack = (uint32_t *)kmalloc_a(TASK_STACK_SIZE);
        if (!t->stack)
            return -1;
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
    t->cr3 = 0;
    t->is_user = 0;
    t->parent = current_id;
    t->user_eip = 0;
    t->user_esp = 0;
    task_init_stdio(t);
    int n = 0;
    while (name && name[n] && n < 15) {
        t->name[n] = name[n];
        n++;
    }
    t->name[n] = 0;
    task_count++;
    printk("[task] #%d '%s'\n", id, t->name);
    return id;
}

int task_create_user(const char *name, uint32_t cr3, uint32_t entry,
                     uint32_t user_esp) {
    /* kernel entry = trampoline; ring3 eip/esp stored on task */
    int id = task_create(name, user_trampoline);
    if (id < 0)
        return -1;
    struct task *t = &tasks[id];
    t->is_user = 1;
    t->cr3 = cr3;
    t->user_eip = entry;
    t->user_esp = user_esp;
    t->parent = current_id;
    task_init_stdio(t);

    if (!t->kstack) {
        t->kstack = (uint32_t *)kmalloc_a(TASK_KSTACK_SIZE);
        if (!t->kstack) {
            t->state = TASK_DEAD;
            return -1;
        }
    }
    t->kstack_top = ((uint32_t)t->kstack + TASK_KSTACK_SIZE) & ~0xFu;
    t->state = TASK_READY;
    return id;
}

struct task *task_current(void) {
    return (current_id >= 0) ? &tasks[current_id] : 0;
}

void task_exit_code(int code) {
    __asm__ volatile("cli");
    if (current_id >= 0) {
        struct task *t = &tasks[current_id];
        t->exit_code = code;
        t->state = TASK_ZOMBIE;

        task_close_all_fds(t);

        if (t->cr3 && t->cr3 != paging_kernel_pd()) {
            uint32_t doomed = t->cr3;
            t->cr3 = 0;
            paging_switch(paging_kernel_pd());
            paging_destroy_address_space(doomed);
        }

        int p = t->parent;
        if (p >= 0 && p < MAX_TASKS && tasks[p].state == TASK_BLOCKED)
            tasks[p].state = TASK_READY;

        if (t->wait_parent >= 0 && t->wait_parent < MAX_TASKS &&
            tasks[t->wait_parent].state == TASK_BLOCKED)
            tasks[t->wait_parent].state = TASK_READY;

        for (int i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].state == TASK_BLOCKED &&
                (tasks[i].waiting_on == current_id || tasks[i].waiting_on == -1))
                tasks[i].state = TASK_READY;
        }
    }
    schedule();
    for (;;)
        __asm__ volatile("hlt");
}

void task_exit(void) { task_exit_code(0); }

void task_sleep_ms(uint32_t ms) {
    if (current_id < 0) {
        uint32_t ticks = ms / 10;
        if (!ticks)
            ticks = 1;
        uint32_t start = timer_ticks();
        while (timer_ticks() - start < ticks)
            __asm__ volatile("hlt");
        return;
    }
    uint32_t ticks = ms / 10;
    if (!ticks)
        ticks = 1;
    tasks[current_id].wake_tick = timer_ticks() + ticks;
    tasks[current_id].state = TASK_SLEEPING;
    schedule();
}

int task_kill(int pid) {
    if (pid < 0 || pid >= MAX_TASKS)
        return -1;
    if (tasks[pid].state == TASK_UNUSED || tasks[pid].state == TASK_DEAD)
        return -1;
    if (pid == 0)
        return -1;
    tasks[pid].exit_code = -9;
    tasks[pid].state = TASK_ZOMBIE;
    task_close_all_fds(&tasks[pid]);

    if (tasks[pid].cr3 && tasks[pid].cr3 != paging_kernel_pd()) {
        uint32_t doomed = tasks[pid].cr3;
        tasks[pid].cr3 = 0;
        uint32_t saved = paging_current_pd();
        paging_destroy_address_space(doomed);
        if (saved == doomed)
            paging_switch(paging_kernel_pd());
        else
            paging_switch(saved);
    }

    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_BLOCKED &&
            (tasks[i].waiting_on == pid || tasks[i].waiting_on == -1))
            tasks[i].state = TASK_READY;
    }
    int p = tasks[pid].parent;
    if (p >= 0 && p < MAX_TASKS && tasks[p].state == TASK_BLOCKED)
        tasks[p].state = TASK_READY;

    if (pid == current_id)
        schedule();
    return 0;
}

int task_wait(int pid, int *status) {
    if (current_id < 0)
        return -1;
    for (;;) {
        int found = -1;
        for (int i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].state != TASK_ZOMBIE)
                continue;
            if (pid >= 0 && i != pid)
                continue;
            /* prefer our children */
            if (pid < 0 && tasks[i].parent != current_id && tasks[i].parent >= 0)
                continue;
            found = i;
            break;
        }
        /* if strict child filter found nothing, accept any zombie when pid<0 */
        if (found < 0 && pid < 0) {
            for (int i = 0; i < MAX_TASKS; i++) {
                if (tasks[i].state == TASK_ZOMBIE) {
                    found = i;
                    break;
                }
            }
        }
        if (found >= 0) {
            if (status)
                *status = tasks[found].exit_code;
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
    if (pid < 0 || pid >= MAX_TASKS)
        return -1;
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
        if (tasks[i].state == TASK_READY) {
            next = i;
            break;
        }
    }
    if (next < 0) {
        if (current_id >= 0 &&
            (tasks[current_id].state == TASK_RUNNING ||
             tasks[current_id].state == TASK_READY))
            return;
        for (int i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].state == TASK_READY || tasks[i].state == TASK_RUNNING) {
                next = i;
                break;
            }
        }
    }
    if (next < 0)
        return;

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

    task_switch_address_space(&tasks[next]);

    if (prev == next)
        return;
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
    printk("ID STATE     TICKS CR3      U BRK      NAME\n");
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED)
            continue;
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
        printk("%2d %s %5u %08x %c %08x %s\n", i, st, tasks[i].ticks_ran,
               tasks[i].cr3, tasks[i].is_user ? 'U' : 'K', tasks[i].brk,
               tasks[i].name);
    }
}
