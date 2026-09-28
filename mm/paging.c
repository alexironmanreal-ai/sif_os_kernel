#include "paging.h"
#include "pmm.h"
#include "printk.h"

static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t first_table[1024] __attribute__((aligned(4096)));

void paging_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_i = virt >> 22;
    uint32_t pt_i = (virt >> 12) & 0x3FF;
    if (!(page_directory[pd_i] & PAGE_PRESENT)) {
        uint32_t frame = pmm_alloc_frame();
        if (!frame) { printk("[paging] OOM page table\n"); return; }
        uint32_t *table = (uint32_t *)frame;
        for (int i = 0; i < 1024; i++) table[i] = 0;
        page_directory[pd_i] = frame | PAGE_PRESENT | PAGE_WRITE;
    }
    uint32_t *table = (uint32_t *)(page_directory[pd_i] & ~0xFFF);
    table[pt_i] = (phys & ~0xFFF) | flags;
}

void paging_init(void) {
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0;
        first_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;
    }
    page_directory[0] = ((uint32_t)first_table) | PAGE_PRESENT | PAGE_WRITE;
    uint32_t pd_phys = (uint32_t)page_directory;
    __asm__ volatile ("mov %0, %%cr3" : : "r"(pd_phys));
    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));
    printk("[paging] identity map 0-4MiB, paging ON\n");
}
