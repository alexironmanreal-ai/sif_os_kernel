#include "stdio.h"
#include "unistd.h"
int main(void) {
    printf("=== SIF userspace hello ===\n");
    printf("stdio fds: stdout=1 stderr=2\n");
    write(STDOUT_FILENO, "raw write stdout\n", 16);
    write(STDERR_FILENO, "raw write stderr\n", 16);
    printf("number %d hex %x\n", 42, 0xabc);
    printf("done — exit(0)\n");
    return 0;
}
