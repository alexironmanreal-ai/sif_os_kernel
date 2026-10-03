#ifndef SIF_USERLAND_H
#define SIF_USERLAND_H
#include <stdint.h>
void userland_init(void);
void userland_run_test(void);
int userland_exec_file(const char *name);
int userland_exec_file_argv(const char *name, int argc, char **argv);
int userland_exec_buf(const void *data, uint32_t size, int argc, char **argv);
void userland_do_exit(void);
#endif
