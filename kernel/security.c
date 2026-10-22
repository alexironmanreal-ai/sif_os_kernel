#include "security.h"
#include "printk.h"
#include "paging.h"
#include "timer.h"
#include "kernel.h"
#include "cpu.h"

static uint32_t rng_state;

uintptr_t __stack_chk_guard;

void __stack_chk_fail(void) {
    panic("STACK SMASH detected (canary)");
}

void security_init(void) {
    uint32_t seed = 0xA5A5C0DEu;
    seed ^= timer_ticks();
    seed ^= (uint32_t)(uintptr_t)&seed;
    {
        uint8_t lo, hi;
        __asm__ volatile("inb $0x40, %0" : "=a"(lo));
        __asm__ volatile("inb $0x40, %0" : "=a"(hi));
        seed ^= ((uint32_t)hi << 8) | lo;
    }
    if (seed == 0)
        seed = 0x12345678u;
    rng_state = seed;

    __stack_chk_guard = (uintptr_t)security_rand32();
    if (__stack_chk_guard == 0)
        __stack_chk_guard = 0xDEADC0DEu;

    cpu_enable_smep_smap();

    printk("[sec] canary=%x aslr-seed=%x\n", (uint32_t)__stack_chk_guard,
           rng_state);
}

uint32_t security_rand32(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x ? x : 0xA5A5A5A5u;
    return rng_state;
}

int security_user_range_ok(uint32_t start, uint32_t len) {
    if (!len)
        return 1;
    uint32_t end = start + len;
    if (end < start)
        return 0;
    if (start < USER_BASE || end > USER_TOP)
        return 0;
    return 1;
}

uint32_t aslr_stack_top(void) {
    uint32_t lo = 0x08080000u;
    uint32_t hi = 0x08E00000u;
    uint32_t pages = (hi - lo) / 4096u;
    uint32_t r = security_rand32() % pages;
    return lo + r * 4096u;
}

uint32_t aslr_brk_start(void) {
    uint32_t lo = 0x08100000u;
    uint32_t hi = 0x08200000u;
    uint32_t pages = (hi - lo) / 4096u;
    uint32_t r = security_rand32() % pages;
    return lo + r * 4096u;
}

void security_audit(void) {
    printk("=== SIF security audit ===\n");
    printk("[+] ring0/ring3 + TSS\n");
    printk("[+] syscall uaccess + path_ok\n");
    printk("[+] user VA [%x, %x)\n", USER_BASE, USER_TOP);
    printk("[+] stack canary %x\n", (uint32_t)__stack_chk_guard);
    printk("[+] ASLR stack/brk + stack guard page\n");
    printk("%s SMEP\n", cpu_smep_enabled() ? "[+]" : "[~]");
    printk("%s SMAP\n", cpu_smap_enabled() ? "[+]" : "[~]");
    printk("%s NX hardware (PAE not enabled — no PTE NX yet)\n",
           cpu_has_nx() ? "[~]" : "[-]");
    printk("[~] ELF not PIE\n");
    printk("=== end audit ===\n");
}
