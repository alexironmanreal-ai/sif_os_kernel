#ifndef SIF_KMALLOC_H
#define SIF_KMALLOC_H
#include <stddef.h>
#include <stdint.h>
void kmalloc_init(void);
void *kmalloc(size_t size);
void *kmalloc_a(size_t size);
void kfree(void *ptr);
#endif
