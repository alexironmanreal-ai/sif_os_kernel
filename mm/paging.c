#include "paging.h"
#include "pmm.h"
#include "printk.h"
#include "string.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

/* kernel page directory + first 4 MiB table (identity) */
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t first_table[1024] __attribute__((aligned(4096)));

static uint32_t *current_pd = page_directory;
static uint32_t kernel_pd_phys;
static uint32_t current_pd_phys;

static inline uint32_t *pd_from_phys(uint32_t phys) {
    return (uint32_t *)phys; /* identity-mapped low memory */
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

    /* invalidate TLB for this page */
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
    if (!frame) {
        printk("[paging] OOM address space\n");
        return 0;
    }
    uint32_t *pd = (uint32_t *)frame;
    for (int i = 0; i < 1024; i++)
        pd[i] = 0;

    /*
     * Clone kernel mappings: share the same page tables for the low
     * identity region and any other kernel PDEs already present.
     * User region (PDE for 0x08000000+) stays empty until exec maps it.
     */
    for (int i = 0; i < 1024; i++) {
        uint32_t entry = page_directory[i];
        if (!(entry & PAGE_PRESENT))
            continue;
        uint32_t va = (uint32_t)i << 22;
        /* do not copy user-process region into the new space */
        if (va >= USER_BASE && va < USER_TOP)
            continue;
        pd[i] = entry; /* share kernel page tables */
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
                uint32_t phys = table[i] & ~0xFFFu;
                pmm_free_frame(phys);
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

    /* free the page directory frame itself */
    pmm_free_frame(pd_phys);
}

void paging_init(void) {
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0;
        first_table[i] = 0;
    }
    for (uint32_t i = 0; i < 1024; i++)
        first_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;

    page_directory[0] = ((uint32_t)first_table) | PAGE_PRESENT | PAGE_WRITE;

    kernel_pd_phys = (uint32_t)page_directory;
    current_pd = page_directory;
    current_pd_phys = kernel_pd_phys;

    __asm__ volatile("mov %0, %%cr3" ::"r"(kernel_pd_phys));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));

    printk("[paging] identity 0-4MiB, addr-spaces ON\n");
}

void page_fault_handler(uint32_t err, uint32_t eip) {
    uint32_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    int present = err & 1, write = err & 2, user = err & 4;
    printk("\n*** PAGE FAULT *** CR2=%x EIP=%x err=%x\n", cr2, eip, err);
    printk("  %s %s %s\n", present ? "protection" : "not-present",
           write ? "write" : "read", user ? "user" : "kernel");

    /* demand-page user heap region */
    if (!present && user && cr2 >= USER_BASE && cr2 < USER_TOP) {
        uint32_t frame = pmm_alloc_frame();
        if (frame) {
            uint32_t va = cr2 & ~0xFFFu;
            paging_map(va, frame, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
            memset((void *)va, 0, PAGE_SIZE);
            printk("[pf] demand-map %x\n", va);
            return;
        }
    }
    for (;;)
        __asm__ volatile("cli; hlt");
}
