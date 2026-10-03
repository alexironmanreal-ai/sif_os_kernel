#include "cmd.h"
#include "printk.h"
#include <stddef.h>
static struct cmd_entry table[CMD_MAX_REG];
static int ncmd = 0;
void cmd_init(void) { ncmd = 0; }
int cmd_register(const char *name, const char *help, cmd_fn fn) {
    if (ncmd >= CMD_MAX_REG || !name || !fn) return -1;
    table[ncmd].name = name;
    table[ncmd].help = help ? help : "";
    table[ncmd].fn = fn;
    ncmd++;
    return 0;
}
void cmd_list(void) {
    printk("Comandos registrados (%d):\n", ncmd);
    for (int i = 0; i < ncmd; i++)
        printk("  %-12s %s\n", table[i].name, table[i].help);
}
int cmd_parse(char *line, int *argc, char **argv) {
    int n = 0;
    while (*line && n < CMD_MAX_ARGS) {
        while (*line == ' ' || *line == '\t') line++;
        if (!*line) break;
        argv[n++] = line;
        while (*line && *line != ' ' && *line != '\t') line++;
        if (*line) { *line = 0; line++; }
    }
    *argc = n;
    return n;
}
int cmd_run_line(char *line) {
    while (*line == ' ' || *line == '\t') line++;
    if (!*line) return 0;
    char *argv[CMD_MAX_ARGS];
    int argc = 0;
    cmd_parse(line, &argc, argv);
    if (argc == 0) return 0;
    for (int i = 0; i < ncmd; i++) {
        const char *a = argv[0], *b = table[i].name;
        while (*a && *b && *a == *b) { a++; b++; }
        if (*a == 0 && *b == 0) return table[i].fn(argc, argv);
    }
    printk("comando desconocido: '%s' (help)\n", argv[0]);
    return -1;
}
