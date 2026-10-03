#include "gdt.h"
#include "printk.h"
static struct gdt_entry gdt[6];
static struct gdt_ptr gp;
static struct tss_entry tss;
extern void gdt_flush(uint32_t);
extern void tss_flush(void);
static void gdt_set(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[i].base_low = base & 0xFFFF;
    gdt[i].base_mid = (base >> 16) & 0xFF;
    gdt[i].base_high = (base >> 24) & 0xFF;
    gdt[i].limit_low = limit & 0xFFFF;
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[i].access = access;
}
static void write_tss(int num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss;
    gdt_set(num, base, sizeof(tss) - 1, 0x89, 0x00);
    for (uint32_t i = 0; i < sizeof(tss) / 4; i++) ((uint32_t *)&tss)[i] = 0;
    tss.ss0 = ss0; tss.esp0 = esp0;
    tss.cs = GDT_KCODE;
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = GDT_KDATA;
    tss.iomap_base = sizeof(tss);
}
void tss_set_kernel_stack(uint32_t esp0) { tss.esp0 = esp0; }
struct tss_entry *tss_get(void) { return &tss; }
void gdt_init(void) {
    gp.limit = sizeof(gdt) - 1;
    gp.base = (uint32_t)&gdt;
    gdt_set(0, 0, 0, 0, 0);
    gdt_set(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    gdt_set(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    gdt_set(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    static uint8_t kstack[4096] __attribute__((aligned(16)));
    write_tss(5, GDT_KDATA, (uint32_t)&kstack[4096]);
    gdt_flush((uint32_t)&gp);
    tss_flush();
    printk("[cpu] GDT ring0+ring3 + TSS listo\n");
}
