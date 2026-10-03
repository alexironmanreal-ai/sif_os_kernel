#include "stdio.h"
#include "unistd.h"
int main(void) {
    int fd = open("readme.txt", 0);
    if (fd < 0) { printf("cat: cannot open readme.txt\n"); return 1; }
    char buf[128]; int n;
    while ((n = read(fd, buf, sizeof(buf)-1)) > 0) {
        buf[n]=0; write(STDOUT_FILENO, buf, (unsigned)n);
    }
    close(fd); return 0;
}
