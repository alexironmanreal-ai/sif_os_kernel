#ifndef SIF_PMM_H
#define SIF_PMM_H
#include <stdint.h>
#include <stddef.h>
#include "multiboot.h"
#define PAGE_SIZE 4096
void pmm_init(struct multiboot_info *mbi, uint32_t kernel_end);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t phys);
uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);
uint32_t pmm_free_frames(void);
#endif
