#ifndef SIF_USERLAND_H
#define SIF_USERLAND_H
#include <stdint.h>
void userland_init(void);
void userland_run_test(void);
/* blocking (legacy): runs to completion in caller context */
int userland_exec_file(const char *name);
int userland_exec_file_argv(const char *name, int argc, char **argv);
int userland_exec_buf(const void *data, uint32_t size, int argc, char **argv);
/* async: create process, return pid (>=0) or -1 */
int userland_spawn_file(const char *name);
int userland_spawn_file_argv(const char *name, int argc, char **argv);
int userland_spawn_buf(const void *data, uint32_t size, int argc, char **argv,
                       const char *name);
void userland_do_exit(void);
#endif
