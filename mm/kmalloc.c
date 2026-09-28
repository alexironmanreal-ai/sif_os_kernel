#include "kmalloc.h"
#include "pmm.h"
#include "printk.h"

#define HEAP_START 0x400000
#define HEAP_MAX   0x800000

static uint32_t heap_pos = HEAP_START;
static uint32_t heap_end = HEAP_START;

void kmalloc_init(void) {
    heap_pos = HEAP_START;
    heap_end = HEAP_START;
    printk("[kmalloc] heap @ %x\n", HEAP_START);
}

static void heap_expand(uint32_t need) {
    while (heap_end < need && heap_end < HEAP_MAX) {
        uint32_t frame = pmm_alloc_frame();
        if (!frame) { printk("[kmalloc] OOM\n"); return; }
        extern void paging_map(uint32_t, uint32_t, uint32_t);
        paging_map(heap_end, frame, 0x3);
        heap_end += PAGE_SIZE;
    }
}

void *kmalloc_a(size_t size) {
    if (size == 0) return 0;
    if (heap_pos & 0xFFF) heap_pos = (heap_pos + PAGE_SIZE) & ~0xFFF;
    return kmalloc(size);
}

void *kmalloc(size_t size) {
    if (size == 0) return 0;
    size = (size + 7) & ~7u;
    uint32_t need = heap_pos + size;
    if (need > heap_end) heap_expand(need);
    if (heap_pos + size > heap_end) return 0;
    void *ptr = (void *)heap_pos;
    heap_pos += size;
    return ptr;
}

void kfree(void *ptr) { (void)ptr; }
