#include "cpu.h"
#include "printk.h"

static int smep_on;
static int smap_on;

int cpu_has_cpuid(void) {
    uint32_t eflags_before, eflags_after;
    __asm__ volatile(
        "pushfl\n"
        "popl %0\n"
        "movl %0, %%eax\n"
        "xorl $0x200000, %%eax\n"
        "pushl %%eax\n"
        "popfl\n"
        "pushfl\n"
        "popl %1\n"
        "pushl %0\n"
        "popfl\n"
        : "=r"(eflags_before), "=r"(eflags_after)
        :
        : "eax", "cc");
    return ((eflags_before ^ eflags_after) & 0x200000) != 0;
}

static void cpuid(uint32_t leaf, uint32_t *a, uint32_t *b, uint32_t *c,
                  uint32_t *d) {
    __asm__ volatile("cpuid"
                     : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
                     : "a"(leaf), "c"(0));
}

void cpu_vendor(char out[13]) {
    uint32_t a, b, c, d;
    if (!cpu_has_cpuid()) {
        out[0] = 0;
        return;
    }
    cpuid(0, &a, &b, &c, &d);
    *(uint32_t *)(out + 0) = b;
    *(uint32_t *)(out + 4) = d;
    *(uint32_t *)(out + 8) = c;
    out[12] = 0;
}

int cpu_has_pae(void) {
    uint32_t a, b, c, d;
    if (!cpu_has_cpuid())
        return 0;
    cpuid(1, &a, &b, &c, &d);
    return (d & (1u << 6)) != 0;
}

int cpu_has_long_mode(void) {
    uint32_t a, b, c, d;
    if (!cpu_has_cpuid())
        return 0;
    cpuid(0x80000000u, &a, &b, &c, &d);
    if (a < 0x80000001u)
        return 0;
    cpuid(0x80000001u, &a, &b, &c, &d);
    return (d & (1u << 29)) != 0;
}

int cpu_has_nx(void) {
    uint32_t a, b, c, d;
    if (!cpu_has_cpuid())
        return 0;
    cpuid(0x80000000u, &a, &b, &c, &d);
    if (a < 0x80000001u)
        return 0;
    cpuid(0x80000001u, &a, &b, &c, &d);
    return (d & (1u << 20)) != 0; /* NX */
}

int cpu_has_smep(void) {
    uint32_t a, b, c, d;
    if (!cpu_has_cpuid())
        return 0;
    cpuid(7, &a, &b, &c, &d);
    return (b & (1u << 7)) != 0; /* SMEP */
}

int cpu_has_smap(void) {
    uint32_t a, b, c, d;
    if (!cpu_has_cpuid())
        return 0;
    cpuid(7, &a, &b, &c, &d);
    return (b & (1u << 20)) != 0; /* SMAP */
}

int cpu_smep_enabled(void) { return smep_on; }
int cpu_smap_enabled(void) { return smap_on; }

void cpu_enable_smep_smap(void) {
    smep_on = 0;
    smap_on = 0;
    uint32_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));

    if (cpu_has_smep()) {
        cr4 |= (1u << 20); /* CR4.SMEP */
        smep_on = 1;
    }
    if (cpu_has_smap()) {
        cr4 |= (1u << 21); /* CR4.SMAP */
        smap_on = 1;
    }

    __asm__ volatile("mov %0, %%cr4" ::"r"(cr4) : "memory");

    if (smep_on || smap_on)
        printk("[cpu] CR4 SMEP=%d SMAP=%d\n", smep_on, smap_on);
    else
        printk("[cpu] SMEP/SMAP no soportado en este CPU\n");

    /* NX on 32-bit non-PAE page tables is not enforceable here */
    if (cpu_has_nx())
        printk("[cpu] NX capable (needs PAE page tables to enforce)\n");
}

void cpu_report(void) {
    char vend[13];
    cpu_vendor(vend);
    printk("cpu vendor: %s\n", vend[0] ? vend : "?");
    printk("  cpuid: %s\n", cpu_has_cpuid() ? "yes" : "no");
    printk("  PAE:   %s\n", cpu_has_pae() ? "yes" : "no");
    printk("  NX:    %s\n", cpu_has_nx() ? "yes" : "no");
    printk("  LM64:  %s\n", cpu_has_long_mode() ? "yes" : "no");
    printk("  SMEP:  %s (on=%d)\n", cpu_has_smep() ? "yes" : "no", smep_on);
    printk("  SMAP:  %s (on=%d)\n", cpu_has_smap() ? "yes" : "no", smap_on);
}
