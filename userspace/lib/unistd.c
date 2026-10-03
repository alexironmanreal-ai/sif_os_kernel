#include "unistd.h"
void _exit(int code) { syscall3(SYS_EXIT, code, 0, 0); for (;;) {} }
void exit(int code) { _exit(code); }
int write(int fd, const void *buf, unsigned n) {
    return syscall3(SYS_WRITE, fd, (int)buf, (int)n);
}
int read(int fd, void *buf, unsigned n) {
    return syscall3(SYS_READ, fd, (int)buf, (int)n);
}
int open(const char *path, int flags) {
    return syscall3(SYS_OPEN, (int)path, flags, 0);
}
int close(int fd) { return syscall3(SYS_CLOSE, fd, 0, 0); }
int getpid(void) { return syscall3(SYS_GETPID, 0, 0, 0); }
void sleep_ms(unsigned ms) { syscall3(SYS_SLEEP, (int)ms, 0, 0); }
int kill(int pid) { return syscall3(SYS_KILL, pid, 0, 0); }
void *sbrk(int incr) {
    int cur = syscall3(SYS_BRK, 0, 0, 0);
    if (incr == 0) return (void *)cur;
    int r = syscall3(SYS_BRK, cur + incr, 0, 0);
    if (r == cur && incr > 0) return (void *)-1;
    return (void *)cur;
}
