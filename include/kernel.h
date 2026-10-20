#ifndef SIF_KERNEL_H
#define SIF_KERNEL_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "kernel_mem.h"
#define SIF_KERNEL_NAME "SIF Kernel"
#define SIF_KERNEL_VERSION "2.7.0-sec"
void kernel_main(uint32_t magic, void *mbi);
void panic(const char *msg);
#endif
