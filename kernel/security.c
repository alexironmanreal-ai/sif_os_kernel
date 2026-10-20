#include "security.h"
#include "printk.h"
#include "paging.h"
#include "timer.h"
#include "kernel.h"
#include "idt.h"
#include "cpu.h"

/* xorshift32 PRNG */
static uint32_t rng_state;

uintptr_t __stack_chk_guard;

void __stack_chk_fail(void) {
    panic("STACK SMASH detected (canary)");
}

void security_init(void) {
    /* mix timer + TSC-ish ports + fixed salt */
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

/*
 * Stack top: somewhere in upper half of user region, page-aligned.
 * Leaves room below for growth (2+ pages).
 */
uint32_t aslr_stack_top(void) {
    /* base 0x08080000 .. 0x08F00000 step 4K, keep clear of code @ 0x08000000 */
    uint32_t lo = 0x08080000u;
    uint32_t hi = 0x08E00000u;
    uint32_t pages = (hi - lo) / 4096u;
    uint32_t r = security_rand32() % pages;
    return lo + r * 4096u;
}

uint32_t aslr_brk_start(void) {
    /* heap window after typical code, before stack area */
    uint32_t lo = 0x08100000u;
    uint32_t hi = 0x08200000u;
    uint32_t pages = (hi - lo) / 4096u;
    uint32_t r = security_rand32() % pages;
    return lo + r * 4096u;
}

void security_audit(void) {
    printk("=== SIF security audit ===\n");

    printk("[+] ring separation: GDT ring0/ring3 + TSS\n");
    printk("[+] IDT 256 gates (no empty vectors)\n");
    printk("[+] syscall 0x80 DPL=3 + uaccess copy_*\n");
    printk("[+] user VA only [%x, %x)\n", USER_BASE, USER_TOP);
    printk("[+] path_ok rejects .. and absolute\n");
    printk("[+] stack canary: guard=%x\n", (uint32_t)__stack_chk_guard);
    printk("[+] ASLR: stack top + brk base per process\n");

    /* isolation checks */
    int ok = 1;
    if (!security_user_range_ok(USER_BASE, 0x1000))
        ok = 0;
    if (security_user_range_ok(0xC0000000u, 0x1000))
        ok = 0; /* kernel VMA must NOT be user-ok */
    if (security_user_range_ok(0x0, 0x1000))
        ok = 0; /* null page not user region */

    printk("%s range policy self-test\n", ok ? "[+]" : "[-]");

    printk("[~] ELF not PIE — load addr fixed (stack/brk still ASLR)\n");
    printk("[~] no SMEP/SMAP yet\n");
    printk("[~] no NX bit enforcement yet\n");
    printk("=== end audit ===\n");
}
