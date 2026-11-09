#ifndef SIF_KERNEL_MEM_H
#define SIF_KERNEL_MEM_H

#include <stdint.h>

#define KERNEL_VMA 0xC0000000u
#define KERNEL_LMA 0x00100000u
/* Must cover QEMU -m 64M so page-table frames are always identity-reachable */
#define IDENTITY_TOP 0x04000000u /* 64 MiB */
#define IDENTITY_PTS 16          /* 16 * 4MiB */

#define V2P(v) ((uint32_t)(v) - KERNEL_VMA)
#define P2V(p) ((uint32_t)(p) + KERNEL_VMA)

#ifndef __ASSEMBLER__
extern char kernel_start[];
extern char kernel_end[];
extern char kernel_phys_start[];
extern char kernel_phys_end[];
#endif

#endif
