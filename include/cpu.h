#ifndef SIF_CPU_H
#define SIF_CPU_H

#include <stdint.h>

int cpu_has_cpuid(void);
int cpu_has_long_mode(void);
int cpu_has_pae(void);
int cpu_has_nx(void);   /* IA32_EFER.NXE capable (needs PAE or LM) */
int cpu_has_smep(void);
int cpu_has_smap(void);
int cpu_smep_enabled(void);
int cpu_smap_enabled(void);
void cpu_enable_smep_smap(void);
void cpu_vendor(char out[13]);
void cpu_report(void);

void longmode_enter(void);

#endif
