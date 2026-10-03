#ifndef SIF_GDT_H
#define SIF_GDT_H
#include <stdint.h>
struct gdt_entry {
    uint16_t limit_low; uint16_t base_low; uint8_t base_mid;
    uint8_t access; uint8_t granularity; uint8_t base_high;
} __attribute__((packed));
struct gdt_ptr { uint16_t limit; uint32_t base; } __attribute__((packed));
struct tss_entry {
    uint32_t prev_tss, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
    uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs, ldt;
    uint16_t trap, iomap_base;
} __attribute__((packed));
#define GDT_KCODE 0x08
#define GDT_KDATA 0x10
#define GDT_UCODE 0x18
#define GDT_UDATA 0x20
#define GDT_TSS 0x28
void gdt_init(void);
void tss_set_kernel_stack(uint32_t esp0);
struct tss_entry *tss_get(void);
#endif
