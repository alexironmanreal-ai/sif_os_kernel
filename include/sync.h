#ifndef SIF_SYNC_H
#define SIF_SYNC_H
#include <stdint.h>

/*
 * Minimal synchronization primitives.
 * mutex: cooperative (blocks via schedule, not spin forever on UP).
 */
struct mutex {
    int locked;
    int owner;          /* task id or -1 */
    int waiters[8];     /* blocked task ids */
    int nwaiters;
};

void mutex_init(struct mutex *m);
void mutex_lock(struct mutex *m);
void mutex_unlock(struct mutex *m);
int mutex_trylock(struct mutex *m);

#endif
