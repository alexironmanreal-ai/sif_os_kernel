#ifndef SIF_KERNEL_H
#define SIF_KERNEL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define SIF_KERNEL_NAME    "SIF Kernel"
#define SIF_KERNEL_VERSION "0.1.0-base"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

void kernel_main(uint32_t magic, void *mbi);
void panic(const char *msg);

#endif
