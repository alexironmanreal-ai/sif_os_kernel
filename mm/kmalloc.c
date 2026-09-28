#include "kmalloc.h"
#include "pmm.h"
#include "paging.h"
#include "printk.h"
#include "string.h"

/* Simple freelist heap allocator.
 * Layout of each block:
 *   [size | magic | next]  header
 *   [ .............. ]     payload
 */

#define HEAP_START   0x00400000u
#define HEAP_MAX     0x01000000u   /* up to 16 MiB virtual */
#define HEAP_MAGIC   0x51F0A110u
#define MIN_SPLIT    32

struct block {
    uint32_t size;          /* total size including header */
    uint32_t magic;
    struct block *next;     /* free list link (only when free) */
};

static struct block *free_list = 0;
static uint32_t heap_end = HEAP_START;
static uint32_t total_allocated = 0;
static uint32_t total_freed = 0;

static void heap_grow(uint32_t need) {
    while (heap_end < need && heap_end < HEAP_MAX) {
        uint32_t frame = pmm_alloc_frame();
        if (!frame) {
            printk("[kmalloc] OOM growing heap\n");
            return;
        }
        paging_map(heap_end, frame, PAGE_PRESENT | PAGE_WRITE);
        heap_end += PAGE_SIZE;
    }
}

static void insert_free(struct block *b) {
    b->magic = HEAP_MAGIC;
    b->next = free_list;
    free_list = b;
}

void kmalloc_init(void) {
    free_list = 0;
    heap_end = HEAP_START;
    total_allocated = 0;
    total_freed = 0;

    /* Seed with a first page */
    heap_grow(HEAP_START + PAGE_SIZE);
    if (heap_end <= HEAP_START) {
        printk("[kmalloc] failed to seed heap\n");
        return;
    }

    struct block *first = (struct block *)HEAP_START;
    first->size = heap_end - HEAP_START;
    first->magic = HEAP_MAGIC;
    first->next = 0;
    free_list = first;

    printk("[kmalloc] freelist heap @ %x (max %x)\n", HEAP_START, HEAP_MAX);
}

void *kmalloc(size_t size) {
    if (size == 0) return 0;
    size = (size + 7) & ~7u;                 /* 8-byte align payload */
    uint32_t need = size + sizeof(struct block);
    if (need < sizeof(struct block) + 8) need = sizeof(struct block) + 8;

    struct block *prev = 0;
    struct block *b = free_list;
    while (b) {
        if (b->magic != HEAP_MAGIC) {
            printk("[kmalloc] corrupt free block @ %x\n", (uint32_t)b);
            return 0;
        }
        if (b->size >= need) break;
        prev = b;
        b = b->next;
    }

    if (!b) {
        /* grow heap */
        uint32_t grow_to = heap_end + need + PAGE_SIZE;
        if (grow_to > HEAP_MAX) grow_to = HEAP_MAX;
        uint32_t old_end = heap_end;
        heap_grow(grow_to);
        if (heap_end <= old_end) return 0;

        struct block *nb = (struct block *)old_end;
        nb->size = heap_end - old_end;
        nb->magic = HEAP_MAGIC;
        nb->next = 0;
        if (prev) prev->next = nb;
        else free_list = nb;
        b = nb;
    }

    /* split if leftover is useful */
    if (b->size >= need + MIN_SPLIT) {
        struct block *rest = (struct block *)((uint8_t *)b + need);
        rest->size = b->size - need;
        rest->magic = HEAP_MAGIC;
        rest->next = b->next;
        b->size = need;
        b->next = rest;
    }

    /* unlink from free list */
    if (prev) prev->next = b->next;
    else free_list = b->next;

    b->next = 0; /* mark used */
    total_allocated += b->size;
    return (void *)(b + 1);
}

void *kmalloc_a(size_t size) {
    if (size == 0) return 0;
    /* over-allocate and align; acceptable for task stacks */
    void *raw = kmalloc(size + PAGE_SIZE);
    if (!raw) return 0;
    uintptr_t addr = ((uintptr_t)raw + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    return (void *)addr;
}

void kfree(void *ptr) {
    if (!ptr) return;
    struct block *b = ((struct block *)ptr) - 1;
    if (b->magic != HEAP_MAGIC) {
        printk("[kfree] bad magic @ %x\n", (uint32_t)b);
        return;
    }
    total_freed += b->size;
    insert_free(b);
}

void kmalloc_stats(void) {
    uint32_t free_bytes = 0;
    int nfree = 0;
    for (struct block *b = free_list; b; b = b->next) {
        free_bytes += b->size;
        nfree++;
    }
    printk("heap: end=%x free_blocks=%d free_bytes=%u alloc=%u freed=%u\n",
           heap_end, nfree, free_bytes, total_allocated, total_freed);
}
