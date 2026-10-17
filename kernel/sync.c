#include "sync.h"
#include "task.h"
#include "printk.h"

void mutex_init(struct mutex *m) {
    if (!m)
        return;
    m->locked = 0;
    m->owner = -1;
    m->nwaiters = 0;
    for (int i = 0; i < 8; i++)
        m->waiters[i] = -1;
}

int mutex_trylock(struct mutex *m) {
    if (!m)
        return -1;
    __asm__ volatile("cli");
    if (m->locked) {
        __asm__ volatile("sti");
        return -1;
    }
    m->locked = 1;
    {
        struct task *t = task_current();
        m->owner = t ? t->id : -1;
    }
    __asm__ volatile("sti");
    return 0;
}

void mutex_lock(struct mutex *m) {
    if (!m)
        return;
    for (;;) {
        __asm__ volatile("cli");
        if (!m->locked) {
            m->locked = 1;
            struct task *t = task_current();
            m->owner = t ? t->id : -1;
            __asm__ volatile("sti");
            return;
        }
        /* enqueue current task and block */
        struct task *t = task_current();
        if (t && m->nwaiters < 8) {
            m->waiters[m->nwaiters++] = t->id;
            t->state = TASK_BLOCKED;
            t->waiting_on = -2; /* mutex wait marker */
        }
        __asm__ volatile("sti");
        schedule();
    }
}

void mutex_unlock(struct mutex *m) {
    if (!m)
        return;
    __asm__ volatile("cli");
    m->locked = 0;
    m->owner = -1;
    if (m->nwaiters > 0) {
        int wid = m->waiters[0];
        for (int i = 1; i < m->nwaiters; i++)
            m->waiters[i - 1] = m->waiters[i];
        m->nwaiters--;
        m->waiters[m->nwaiters] = -1;
        /* wake waiter — task_wake by id via state READY */
        extern void task_set_ready(int id);
        task_set_ready(wid);
    }
    __asm__ volatile("sti");
}
