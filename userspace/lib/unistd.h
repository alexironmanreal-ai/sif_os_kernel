#ifndef USER_UNISTD_H
#define USER_UNISTD_H
#define SYS_EXIT 1
#define SYS_WRITE 2
#define SYS_YIELD 3
#define SYS_GETPID 4
#define SYS_SLEEP 5
#define SYS_TIME 6
#define SYS_READ 7
#define SYS_OPEN 8
#define SYS_CLOSE 9
#define SYS_KILL 10
#define SYS_WAIT 11
#define SYS_BRK 12
#define SYS_GETDENTS 13
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
int syscall3(int num, int a, int b, int c);
void _exit(int code);
void exit(int code);
int write(int fd, const void *buf, unsigned n);
int read(int fd, void *buf, unsigned n);
int open(const char *path, int flags);
int close(int fd);
int getpid(void);
void sleep_ms(unsigned ms);
int kill(int pid);
void *sbrk(int incr);
int getdents(char *buf, unsigned n);
#endif
