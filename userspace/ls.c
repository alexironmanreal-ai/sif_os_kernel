#include "stdio.h"
#include "unistd.h"
static const char *names[] = {"readme.txt","hello.txt","motd","hello.elf","echo.elf","cat.elf","ls.elf",0};
int main(void) {
    printf("NAME\n");
    for (int i=0; names[i]; i++) {
        int fd = open(names[i], 0);
        if (fd >= 0) { printf("  %s\n", names[i]); close(fd); }
    }
    return 0;
}
