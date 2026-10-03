#ifndef SIF_KMALLOC_H
#define SIF_KMALLOC_H
#include <stddef.h>
#include <stdint.h>
void  kmalloc_init(void);
void *kmalloc(size_t size);
void *kmalloc_a(size_t size);
void  kfree(void *ptr);
void  kmalloc_stats(uint32_t *used, uint32_t *free_bytes, uint32_t *blocks);
#endif
