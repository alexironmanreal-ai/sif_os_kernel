#include "stdio.h"
#include "unistd.h"
int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("echo: type a line:\n");
        char buf[128];
        fgets(buf, sizeof(buf));
        printf("%s", buf);
        return 0;
    }
    for (int i = 1; i < argc; i++) {
        if (i > 1) putchar(' ');
        printf("%s", argv[i]);
    }
    putchar('\n');
    return 0;
}
