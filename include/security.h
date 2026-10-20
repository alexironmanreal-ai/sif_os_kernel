#ifndef SIF_SECURITY_H
#define SIF_SECURITY_H

#include <stdint.h>

void security_init(void);
uint32_t security_rand32(void);
void security_audit(void);

/* stack canary (GCC -fstack-protector) */
extern uintptr_t __stack_chk_guard;
void __stack_chk_fail(void);

/* ASLR helpers: page-aligned slides inside user region */
uint32_t aslr_stack_top(void);   /* randomized user stack top */
uint32_t aslr_brk_start(void);   /* randomized heap base */

int security_user_range_ok(uint32_t start, uint32_t len);

#endif
