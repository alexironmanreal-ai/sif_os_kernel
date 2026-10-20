#ifndef SIF_KERNEL_MEM_H
#define SIF_KERNEL_MEM_H

#include <stdint.h>

/* Higher-half: kernel virtual base */
#define KERNEL_VMA      0xC0000000u
#define KERNEL_LMA      0x00100000u /* physical load (1 MiB) */

#define V2P(v)          ((uint32_t)(v) - KERNEL_VMA)
#define P2V(p)          ((uint32_t)(p) + KERNEL_VMA)

/* identity-mapped window for low MMIO / early phys access */
#define IDENTITY_TOP    0x01000000u /* 16 MiB */

#ifndef __ASSEMBLER__
extern uint32_t kernel_start;
extern uint32_t kernel_end;
extern uint32_t kernel_phys_start;
extern uint32_t kernel_phys_end;
#endif

#endif
