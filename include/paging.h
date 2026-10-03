#ifndef SIF_PAGING_H
#define SIF_PAGING_H
#include <stdint.h>
#define PAGE_PRESENT 0x1
#define PAGE_WRITE 0x2
#define PAGE_USER 0x4
void paging_init(void);
void paging_map(uint32_t virt, uint32_t phys, uint32_t flags);
void paging_unmap_user_bit_low(void);
void page_fault_handler(uint32_t err, uint32_t eip);
#endif
