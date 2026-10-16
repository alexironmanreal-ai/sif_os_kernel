#include "stdio.h"
#include "unistd.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    char buf[1024];
    int n = getdents(buf, sizeof(buf) - 1);
    if (n < 0) {
        printf("ls: getdents failed\n");
        return 1;
    }
    buf[n] = 0;

    int count = 0;
    int i = 0;
    while (i < n) {
        /* skip empty lines */
        if (buf[i] == '\n') {
            i++;
            continue;
        }
        int start = i;
        while (i < n && buf[i] != '\n')
            i++;
        /* print name */
        for (int j = start; j < i; j++)
            putchar(buf[j]);
        putchar('\n');
        count++;
        if (i < n)
            i++; /* skip newline */
    }

    if (count == 0)
        printf("(empty)\n");
    else
        printf("%d file(s)\n", count);

    return 0;
}
