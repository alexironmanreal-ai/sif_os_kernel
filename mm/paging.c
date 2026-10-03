#include "paging.h"
#include "pmm.h"
#include "printk.h"
#include "string.h"
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t first_table[1024] __attribute__((aligned(4096)));
void paging_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_i = virt >> 22;
    uint32_t pt_i = (virt >> 12) & 0x3FF;
    if (!(page_directory[pd_i] & PAGE_PRESENT)) {
        uint32_t frame = pmm_alloc_frame();
        if (!frame) { printk("[paging] sin frames\n"); return; }
        uint32_t *table = (uint32_t *)frame;
        for (int i = 0; i < 1024; i++) table[i] = 0;
        uint32_t pdflags = PAGE_PRESENT | PAGE_WRITE;
        if (flags & PAGE_USER) pdflags |= PAGE_USER;
        page_directory[pd_i] = frame | pdflags;
    } else if (flags & PAGE_USER) {
        page_directory[pd_i] |= PAGE_USER;
    }
    uint32_t *table = (uint32_t *)(page_directory[pd_i] & ~0xFFFu);
    table[pt_i] = (phys & ~0xFFFu) | flags;
}
void paging_init(void) {
    for (int i = 0; i < 1024; i++) { page_directory[i] = 0; first_table[i] = 0; }
    for (uint32_t i = 0; i < 1024; i++)
        first_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    page_directory[0] = ((uint32_t)first_table) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    uint32_t pd_phys = (uint32_t)page_directory;
    __asm__ volatile ("mov %0, %%cr3" : : "r"(pd_phys));
    uint32_t cr0; __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));
    printk("[paging] identity 0-4MiB USER+kernel, PG ON\n");
}
