#include "paging.h"
#include "pmm.h"
#include "printk.h"
#include "string.h"
#include "kernel_mem.h"
#include "task.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t ident_tables[IDENTITY_PTS][1024] __attribute__((aligned(4096)));
static uint32_t hh_tables[IDENTITY_PTS][1024] __attribute__((aligned(4096)));

static uint32_t *current_pd = page_directory;
static uint32_t kernel_pd_phys;
static uint32_t current_pd_phys;

static inline uint32_t virt_to_phys_low(uint32_t v) {
    if (v >= KERNEL_VMA)
        return V2P(v);
    return v;
}

/* Page-table frames live in identity window — access by physical address */
static inline uint32_t *pt_virt(uint32_t pt_phys) {
    pt_phys &= ~0xFFFu;
    if (pt_phys < IDENTITY_TOP)
        return (uint32_t *)pt_phys;
    /* Fallback HH window for low kernel objects only */
    if (pt_phys < IDENTITY_TOP)
        return (uint32_t *)P2V(pt_phys);
    printk("[paging] PT phys %x outside identity!\n", pt_phys);
    return (uint32_t *)pt_phys;
}

void paging_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_i = virt >> 22;
    uint32_t pt_i = (virt >> 12) & 0x3FF;

    if (!(current_pd[pd_i] & PAGE_PRESENT)) {
        uint32_t frame = pmm_alloc_frame();
        if (!frame || frame >= IDENTITY_TOP) {
            /* retry for a low frame */
            if (frame && frame >= IDENTITY_TOP)
                pmm_free_frame(frame);
            frame = 0;
            for (uint32_t f = 0x100000u / PAGE_SIZE; f < IDENTITY_TOP / PAGE_SIZE;
                 f++) {
                /* use pmm only via alloc — simplified: require low */
                frame = pmm_alloc_frame();
                if (!frame)
                    break;
                if (frame < IDENTITY_TOP)
                    break;
                pmm_free_frame(frame);
                frame = 0;
            }
        }
        if (!frame || frame >= IDENTITY_TOP) {
            printk("[paging] OOM low PT\n");
            return;
        }
        uint32_t *table = pt_virt(frame);
        for (int i = 0; i < 1024; i++)
            table[i] = 0;
        uint32_t pdflags = PAGE_PRESENT | PAGE_WRITE;
        if (flags & PAGE_USER)
            pdflags |= PAGE_USER;
        current_pd[pd_i] = frame | pdflags;
    } else if (flags & PAGE_USER) {
        current_pd[pd_i] |= PAGE_USER;
    }

    uint32_t *table = pt_virt(current_pd[pd_i]);
    table[pt_i] = (phys & ~0xFFFu) | flags;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void paging_unmap(uint32_t virt) {
    uint32_t pd_i = virt >> 22;
    uint32_t pt_i = (virt >> 12) & 0x3FF;
    if (!(current_pd[pd_i] & PAGE_PRESENT))
        return;
    uint32_t *table = pt_virt(current_pd[pd_i]);
    table[pt_i] = 0;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

uint32_t paging_kernel_pd(void) { return kernel_pd_phys; }
uint32_t paging_current_pd(void) { return current_pd_phys; }

uint32_t paging_create_address_space(void) {
    uint32_t frame = pmm_alloc_frame();
    if (!frame || frame >= IDENTITY_TOP) {
        if (frame)
            pmm_free_frame(frame);
        /* force low */
        for (int tries = 0; tries < 64; tries++) {
            frame = pmm_alloc_frame();
            if (!frame)
                return 0;
            if (frame < IDENTITY_TOP)
                break;
            pmm_free_frame(frame);
            frame = 0;
        }
        if (!frame)
            return 0;
    }
    uint32_t *pd = pt_virt(frame);
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
    /* kernel PD is in HH BSS; user PDs are identity phys */
    if (pd_phys == kernel_pd_phys)
        current_pd = page_directory;
    else
        current_pd = pt_virt(pd_phys);
    __asm__ volatile("mov %0, %%cr3" ::"r"(pd_phys) : "memory");
}

void paging_free_user_region(void) {
    uint32_t start_pde = USER_BASE >> 22;
    uint32_t end_pde = (USER_TOP - 1) >> 22;
    for (uint32_t pde = start_pde; pde <= end_pde; pde++) {
        if (!(current_pd[pde] & PAGE_PRESENT))
            continue;
        uint32_t pt_phys = current_pd[pde] & ~0xFFFu;
        uint32_t *table = pt_virt(pt_phys);
        for (int i = 0; i < 1024; i++) {
            if (table[i] & PAGE_PRESENT) {
                pmm_free_frame(table[i] & ~0xFFFu);
                table[i] = 0;
            }
        }
        pmm_free_frame(pt_phys);
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

    for (int t = 0; t < IDENTITY_PTS; t++) {
        for (uint32_t i = 0; i < 1024; i++) {
            uint32_t phys = (uint32_t)t * 0x400000u + i * PAGE_SIZE;
            ident_tables[t][i] = phys | PAGE_PRESENT | PAGE_WRITE;
            hh_tables[t][i] = phys | PAGE_PRESENT | PAGE_WRITE;
        }
        page_directory[t] =
            virt_to_phys_low((uint32_t)&ident_tables[t][0]) | PAGE_PRESENT |
            PAGE_WRITE;
        /* HH PDE index: 0xC0000000 >> 22 = 768 */
        page_directory[768 + t] =
            virt_to_phys_low((uint32_t)&hh_tables[t][0]) | PAGE_PRESENT |
            PAGE_WRITE;
    }

    /* Null-pointer trap at VA 0 only (phys 0 still in HH for safety) */
    ident_tables[0][0] = 0;

    kernel_pd_phys = virt_to_phys_low((uint32_t)page_directory);
    current_pd = page_directory;
    current_pd_phys = kernel_pd_phys;

    __asm__ volatile("mov %0, %%cr3" ::"r"(kernel_pd_phys) : "memory");

    printk("[paging] identity 0-%uMiB HH VMA=%x\n", IDENTITY_TOP / 0x100000u,
           KERNEL_VMA);
}

void page_fault_handler(uint32_t err, uint32_t eip) {
    uint32_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    printk("\n*** PAGE FAULT *** CR2=%x EIP=%x err=%x\n", cr2, eip, err);

    if (err & 4) { /* user */
        struct task *t = task_current();
        if (t && t->is_user) {
            task_exit_code(-11);
            return;
        }
    }
    for (;;)
        __asm__ volatile("cli; hlt");
}
