#ifndef SIF_CPU_H
#define SIF_CPU_H

#include <stdint.h>

int cpu_has_cpuid(void);
int cpu_has_long_mode(void);
int cpu_has_pae(void);
void cpu_vendor(char out[13]);
void cpu_report(void);

/* one-way: switch to long mode and jump to entry64 (does not return) */
void longmode_enter(void);

#endif
