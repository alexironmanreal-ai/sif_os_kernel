#ifndef SIF_PAGING_H
#define SIF_PAGING_H
#include <stdint.h>

#define PAGE_PRESENT 0x1
#define PAGE_WRITE   0x2
#define PAGE_USER    0x4

/* user virtual region reserved for processes */
#define USER_BASE    0x08000000u
#define USER_TOP     0x09000000u

void paging_init(void);

/* map into the *current* address space */
void paging_map(uint32_t virt, uint32_t phys, uint32_t flags);
void paging_unmap(uint32_t virt);

/* kernel page directory (physical) */
uint32_t paging_kernel_pd(void);

/* current CR3 / page directory physical */
uint32_t paging_current_pd(void);

/*
 * Create a new address space for a user process:
 *  - clones kernel mappings (identity low memory + heap)
 *  - user region starts empty
 * Returns physical address of the new page directory, or 0 on failure.
 */
uint32_t paging_create_address_space(void);

/* switch CR3 to pd_phys (must be identity-accessible) */
void paging_switch(uint32_t pd_phys);

/*
 * Free all user-region page tables and frames in this address space,
 * then free the page directory itself.
 * Does NOT switch away — caller must switch to kernel first if needed.
 */
void paging_destroy_address_space(uint32_t pd_phys);

/* unmap + free every present page in [USER_BASE, USER_TOP) of current PD */
void paging_free_user_region(void);

void page_fault_handler(uint32_t err, uint32_t eip);

#endif
