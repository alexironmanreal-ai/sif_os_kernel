#ifndef SIF_KERNEL_MEM_H
#define SIF_KERNEL_MEM_H

#include <stdint.h>

#define KERNEL_VMA      0xC0000000u
#define KERNEL_LMA      0x00100000u
#define IDENTITY_TOP    0x01000000u /* 16 MiB identity window */

#define V2P(v) ((uint32_t)(v) - KERNEL_VMA)
#define P2V(p) ((uint32_t)(p) + KERNEL_VMA)

#ifndef __ASSEMBLER__
extern char kernel_start[];
extern char kernel_end[];
extern char kernel_phys_start[];
extern char kernel_phys_end[];
#endif

#endif
