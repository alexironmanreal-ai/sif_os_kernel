#include "stdio.h"
#include "unistd.h"
int main(int argc, char **argv) {
    printf("=== SIF userspace hello ===\n");
    printf("argc=%d pid=%d\n", argc, getpid());
    for (int i = 0; i < argc; i++)
        printf("  argv[%d]=%s\n", i, argv[i] ? argv[i] : "(null)");
    write(STDOUT_FILENO, "raw stdout\n", 11);
    printf("done\n");
    return 0;
}
