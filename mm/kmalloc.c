#include "kmalloc.h"
#include "pmm.h"
#include "paging.h"
#include "printk.h"

#define HEAP_START 0x400000
#define HEAP_MAX   0xC00000
#define BLOCK_MAGIC 0xBEEF1234u

struct block {
    uint32_t magic;
    uint32_t size;
    uint32_t free;
    struct block *next;
};

static struct block *head = 0;
static uint32_t heap_end = HEAP_START;
static uint32_t total_used = 0;

static void heap_grow(uint32_t need) {
    while (heap_end < need && heap_end < HEAP_MAX) {
        uint32_t frame = pmm_alloc_frame();
        if (!frame) {
            printk("[kmalloc] OOM grow\n");
            return;
        }
        paging_map(heap_end, frame, PAGE_PRESENT | PAGE_WRITE);
        heap_end += PAGE_SIZE;
    }
}

void kmalloc_init(void) {
    heap_end = HEAP_START;
    heap_grow(HEAP_START + PAGE_SIZE * 4);
    head = (struct block *)HEAP_START;
    head->magic = BLOCK_MAGIC;
    head->size = (heap_end - HEAP_START) - sizeof(struct block);
    head->free = 1;
    head->next = 0;
    total_used = 0;
    printk("[kmalloc] freelist heap %x-%x\n", HEAP_START, heap_end);
}

static void split(struct block *b, uint32_t size) {
    if (b->size < size + sizeof(struct block) + 16)
        return;
    struct block *n = (struct block *)((uint8_t *)b + sizeof(struct block) + size);
    n->magic = BLOCK_MAGIC;
    n->size = b->size - size - sizeof(struct block);
    n->free = 1;
    n->next = b->next;
    b->size = size;
    b->next = n;
}

void *kmalloc(size_t size) {
    if (size == 0) return 0;
    size = (size + 7) & ~7u;
    for (;;) {
        for (struct block *b = head; b; b = b->next) {
            if (b->magic != BLOCK_MAGIC) {
                printk("[kmalloc] heap corrupt\n");
                return 0;
            }
            if (b->free && b->size >= size) {
                split(b, size);
                b->free = 0;
                total_used += b->size;
                return (void *)(b + 1);
            }
        }
        uint32_t old = heap_end;
        heap_grow(heap_end + PAGE_SIZE * 4);
        if (heap_end == old) return 0;
        struct block *tail = head;
        while (tail->next) tail = tail->next;
        if (tail->free) {
            tail->size += (heap_end - old);
        } else {
            struct block *n = (struct block *)old;
            n->magic = BLOCK_MAGIC;
            n->size = (heap_end - old) - sizeof(struct block);
            n->free = 1;
            n->next = 0;
            tail->next = n;
        }
    }
}

void *kmalloc_a(size_t size) {
    uint8_t *p = (uint8_t *)kmalloc(size + PAGE_SIZE);
    if (!p) return 0;
    uint32_t addr = (uint32_t)p;
    return (void *)((addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
}

void kfree(void *ptr) {
    if (!ptr) return;
    struct block *b = ((struct block *)ptr) - 1;
    if (b->magic != BLOCK_MAGIC || b->free) return;
    b->free = 1;
    total_used -= b->size;
    while (b->next && b->next->free) {
        b->size += sizeof(struct block) + b->next->size;
        b->next = b->next->next;
    }
}

void kmalloc_stats(uint32_t *used, uint32_t *free_bytes, uint32_t *blocks) {
    uint32_t f = 0, n = 0;
    for (struct block *b = head; b; b = b->next) {
        n++;
        if (b->free) f += b->size;
    }
    if (used) *used = total_used;
    if (free_bytes) *free_bytes = f;
    if (blocks) *blocks = n;
}
