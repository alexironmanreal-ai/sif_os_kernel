#include "stdio.h"
#include "unistd.h"
int main(void) {
    printf("echo: type a line (stdin):\n");
    char buf[128];
    fgets(buf, sizeof(buf));
    printf("you said: %s", buf);
    return 0;
}
