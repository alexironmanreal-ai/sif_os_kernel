#include "stdio.h"
#include "unistd.h"
int main(int argc, char **argv) {
    (void)argc; (void)argv;
    char buf[512];
    int n = getdents(buf, sizeof(buf) - 1);
    if (n < 0) { printf("ls: error\n"); return 1; }
    buf[n] = 0;
    printf("%s", buf);
    return 0;
}
