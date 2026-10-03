#include "stdio.h"
#include "unistd.h"
int main(int argc, char **argv) {
    const char *name = (argc >= 2) ? argv[1] : "readme.txt";
    int fd = open(name, 0);
    if (fd < 0) { printf("cat: cannot open %s\n", name); return 1; }
    char buf[128]; int n;
    while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = 0;
        write(STDOUT_FILENO, buf, (unsigned)n);
    }
    close(fd);
    return 0;
}
