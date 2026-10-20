#include "cpu.h"
#include "printk.h"

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
                     : "a"(leaf));
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
    return (d & (1u << 6)) != 0; /* PAE */
}

int cpu_has_long_mode(void) {
    uint32_t a, b, c, d;
    if (!cpu_has_cpuid())
        return 0;
    cpuid(0x80000000u, &a, &b, &c, &d);
    if (a < 0x80000001u)
        return 0;
    cpuid(0x80000001u, &a, &b, &c, &d);
    return (d & (1u << 29)) != 0; /* LM */
}

void cpu_report(void) {
    char vend[13];
    cpu_vendor(vend);
    printk("cpu vendor: %s\n", vend[0] ? vend : "?");
    printk("  cpuid: %s\n", cpu_has_cpuid() ? "yes" : "no");
    printk("  PAE:   %s\n", cpu_has_pae() ? "yes" : "no");
    printk("  LM64:  %s\n", cpu_has_long_mode() ? "yes" : "no");
    printk("  entry: 32-bit Multiboot (LM via 'lm')\n");
}
