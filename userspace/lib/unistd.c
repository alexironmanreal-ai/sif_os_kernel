#include "unistd.h"
void _exit(int code) { syscall3(SYS_EXIT, code, 0, 0); for(;;){} }
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
