#ifndef SIF_CMD_H
#define SIF_CMD_H
#include <stdint.h>
#define CMD_MAX_ARGS 16
#define CMD_MAX_NAME 24
#define CMD_MAX_REG 48
typedef int (*cmd_fn)(int argc, char **argv);
struct cmd_entry {
    const char *name;
    const char *help;
    cmd_fn fn;
};
void cmd_init(void);
int cmd_register(const char *name, const char *help, cmd_fn fn);
int cmd_run_line(char *line);
void cmd_list(void);
int cmd_parse(char *line, int *argc, char **argv);
#endif
