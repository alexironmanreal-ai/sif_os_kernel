#include "pmm.h"
#include "printk.h"
#include <stddef.h>

#define MAX_FRAMES (1024 * 1024)
static uint8_t bitmap[MAX_FRAMES / 8];
static uint32_t nframes = 0;
static uint32_t used = 0;

static inline void bit_set(uint32_t f)   { bitmap[f / 8] |=  (1u << (f % 8)); }
static inline void bit_clear(uint32_t f) { bitmap[f / 8] &= ~(1u << (f % 8)); }
static inline int  bit_test(uint32_t f)  { return bitmap[f / 8] & (1u << (f % 8)); }

static void mark_region(uint32_t addr, uint32_t len, int used_flag) {
    uint32_t start = addr / PAGE_SIZE;
    uint32_t end   = (addr + len + PAGE_SIZE - 1) / PAGE_SIZE;
    if (end > nframes) end = nframes;
    for (uint32_t f = start; f < end; f++) {
        if (used_flag) {
            if (!bit_test(f)) { bit_set(f); used++; }
        } else {
            if (bit_test(f)) { bit_clear(f); used--; }
        }
    }
}

void pmm_init(struct multiboot_info *mbi, uint32_t kernel_end) {
    for (uint32_t i = 0; i < sizeof(bitmap); i++) bitmap[i] = 0xFF;
    used = 0; nframes = 0;

    if (!mbi || !(mbi->flags & MULTIBOOT_INFO_MMAP)) {
        uint32_t mem_kb = 1024 + (mbi ? mbi->mem_upper : 7 * 1024);
        nframes = (mem_kb * 1024) / PAGE_SIZE;
        if (nframes > MAX_FRAMES) nframes = MAX_FRAMES;
        for (uint32_t f = 0; f < nframes; f++) bit_clear(f);
        used = 0;
        printk("[pmm] fallback %u frames (~%u MiB)\n", nframes, (nframes * PAGE_SIZE) / (1024*1024));
    } else {
        struct multiboot_mmap_entry *e = (struct multiboot_mmap_entry *)mbi->mmap_addr;
        struct multiboot_mmap_entry *end = (struct multiboot_mmap_entry *)(mbi->mmap_addr + mbi->mmap_length);
        uint32_t max_addr = 0;
        for (struct multiboot_mmap_entry *m = e; m < end;
             m = (struct multiboot_mmap_entry *)((uint32_t)m + m->size + 4)) {
            if (m->type == MULTIBOOT_MEMORY_AVAILABLE) {
                uint32_t top = m->addr_low + m->len_low;
                if (top > max_addr) max_addr = top;
            }
        }
        nframes = max_addr / PAGE_SIZE;
        if (nframes > MAX_FRAMES) nframes = MAX_FRAMES;
        used = nframes;
        for (uint32_t f = 0; f < nframes; f++) bit_set(f);
        for (struct multiboot_mmap_entry *m = e; m < end;
             m = (struct multiboot_mmap_entry *)((uint32_t)m + m->size + 4)) {
            if (m->type == MULTIBOOT_MEMORY_AVAILABLE && m->addr_high == 0) {
                mark_region(m->addr_low, m->len_low, 0);
                printk("[pmm] free  %x +%x\n", m->addr_low, m->len_low);
            }
        }
    }
    mark_region(0, 0x100000, 1);
    mark_region(0x100000, kernel_end > 0x100000 ? kernel_end - 0x100000 : 0x100000, 1);
    printk("[pmm] frames=%u used=%u free=%u\n", nframes, used, nframes - used);
}

uint32_t pmm_alloc_frame(void) {
    for (uint32_t f = 0; f < nframes; f++) {
        if (!bit_test(f)) { bit_set(f); used++; return f * PAGE_SIZE; }
    }
    return 0;
}

void pmm_free_frame(uint32_t phys) {
    uint32_t f = phys / PAGE_SIZE;
    if (f < nframes && bit_test(f)) { bit_clear(f); used--; }
}

uint32_t pmm_total_frames(void) { return nframes; }
uint32_t pmm_used_frames(void)  { return used; }
uint32_t pmm_free_frames(void)  { return nframes - used; }
