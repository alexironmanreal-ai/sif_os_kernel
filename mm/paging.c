#include "paging.h"
#include "pmm.h"
#include "printk.h"
#include "string.h"
#include "kernel_mem.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t ident_tables[4][1024] __attribute__((aligned(4096))); /* 16 MiB */
static uint32_t hh_tables[4][1024] __attribute__((aligned(4096)));    /* 16 MiB HH */

static uint32_t *current_pd = page_directory;
static uint32_t kernel_pd_phys;
static uint32_t current_pd_phys;

static inline uint32_t virt_to_phys_low(uint32_t v) {
    /* kernel objects live in HH; identity window is 1:1 */
    if (v >= KERNEL_VMA)
        return V2P(v);
    return v;
}

static inline uint32_t *pd_from_phys(uint32_t phys) {
    if (phys >= KERNEL_VMA)
        return (uint32_t *)phys;
    /* PD frames allocated from PMM are in identity low region */
    return (uint32_t *)phys;
}

void paging_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_i = virt >> 22;
    uint32_t pt_i = (virt >> 12) & 0x3FF;

    if (!(current_pd[pd_i] & PAGE_PRESENT)) {
        uint32_t frame = pmm_alloc_frame();
        if (!frame) {
            printk("[paging] OOM table\n");
            return;
        }
        uint32_t *table = (uint32_t *)frame;
        for (int i = 0; i < 1024; i++)
            table[i] = 0;
        uint32_t pdflags = PAGE_PRESENT | PAGE_WRITE;
        if (flags & PAGE_USER)
            pdflags |= PAGE_USER;
        current_pd[pd_i] = frame | pdflags;
    } else if (flags & PAGE_USER) {
        current_pd[pd_i] |= PAGE_USER;
    }

    uint32_t *table = (uint32_t *)(current_pd[pd_i] & ~0xFFFu);
    table[pt_i] = (phys & ~0xFFFu) | flags;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void paging_unmap(uint32_t virt) {
    uint32_t pd_i = virt >> 22;
    uint32_t pt_i = (virt >> 12) & 0x3FF;
    if (!(current_pd[pd_i] & PAGE_PRESENT))
        return;
    uint32_t *table = (uint32_t *)(current_pd[pd_i] & ~0xFFFu);
    table[pt_i] = 0;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

uint32_t paging_kernel_pd(void) { return kernel_pd_phys; }
uint32_t paging_current_pd(void) { return current_pd_phys; }

uint32_t paging_create_address_space(void) {
    uint32_t frame = pmm_alloc_frame();
    if (!frame)
        return 0;
    uint32_t *pd = (uint32_t *)frame;
    for (int i = 0; i < 1024; i++)
        pd[i] = 0;

    for (int i = 0; i < 1024; i++) {
        uint32_t entry = page_directory[i];
        if (!(entry & PAGE_PRESENT))
            continue;
        uint32_t va = (uint32_t)i << 22;
        if (va >= USER_BASE && va < USER_TOP)
            continue;
        pd[i] = entry;
    }
    return frame;
}

void paging_switch(uint32_t pd_phys) {
    if (!pd_phys)
        pd_phys = kernel_pd_phys;
    current_pd_phys = pd_phys;
    current_pd = pd_from_phys(pd_phys);
    __asm__ volatile("mov %0, %%cr3" ::"r"(pd_phys) : "memory");
}

void paging_free_user_region(void) {
    uint32_t start_pde = USER_BASE >> 22;
    uint32_t end_pde = (USER_TOP - 1) >> 22;
    for (uint32_t pde = start_pde; pde <= end_pde; pde++) {
        if (!(current_pd[pde] & PAGE_PRESENT))
            continue;
        uint32_t *table = (uint32_t *)(current_pd[pde] & ~0xFFFu);
        for (int i = 0; i < 1024; i++) {
            if (table[i] & PAGE_PRESENT) {
                pmm_free_frame(table[i] & ~0xFFFu);
                table[i] = 0;
            }
        }
        pmm_free_frame((uint32_t)table);
        current_pd[pde] = 0;
    }
}

void paging_destroy_address_space(uint32_t pd_phys) {
    if (!pd_phys || pd_phys == kernel_pd_phys)
        return;
    uint32_t saved = current_pd_phys;
    paging_switch(pd_phys);
    paging_free_user_region();
    paging_switch(saved);
    pmm_free_frame(pd_phys);
}

void paging_init(void) {
    for (int i = 0; i < 1024; i++)
        page_directory[i] = 0;

    /* identity 0-16MiB + HH 0xC0000000-0xC1000000 -> same phys */
    for (int t = 0; t < 4; t++) {
        for (uint32_t i = 0; i < 1024; i++) {
            uint32_t phys = (uint32_t)t * 0x400000u + i * PAGE_SIZE;
            ident_tables[t][i] = phys | PAGE_PRESENT | PAGE_WRITE;
            hh_tables[t][i] = phys | PAGE_PRESENT | PAGE_WRITE;
        }
        page_directory[t] =
            virt_to_phys_low((uint32_t)&ident_tables[t][0]) | PAGE_PRESENT |
            PAGE_WRITE;
        page_directory[768 + t] =
            virt_to_phys_low((uint32_t)&hh_tables[t][0]) | PAGE_PRESENT |
            PAGE_WRITE;
    }

    kernel_pd_phys = virt_to_phys_low((uint32_t)page_directory);
    current_pd = page_directory;
    current_pd_phys = kernel_pd_phys;

    __asm__ volatile("mov %0, %%cr3" ::"r"(kernel_pd_phys) : "memory");

    printk("[paging] higher-half VMA=%x identity 0-16MiB\n", KERNEL_VMA);
}

void page_fault_handler(uint32_t err, uint32_t eip) {
    uint32_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    int present = err & 1, write = err & 2, user = err & 4;
    printk("\n*** PAGE FAULT *** CR2=%x EIP=%x err=%x\n", cr2, eip, err);
    printk("  %s %s %s\n", present ? "protection" : "not-present",
           write ? "write" : "read", user ? "user" : "kernel");

    if (!present && user && cr2 >= USER_BASE && cr2 < USER_TOP) {
        uint32_t frame = pmm_alloc_frame();
        if (frame) {
            uint32_t va = cr2 & ~0xFFFu;
            paging_map(va, frame, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
            memset((void *)va, 0, PAGE_SIZE);
            return;
        }
    }
    for (;;)
        __asm__ volatile("cli; hlt");
}
