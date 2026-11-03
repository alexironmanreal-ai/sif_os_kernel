# SIF Security

## Controles activos (3.0)

1. **Ring separation** — GDT ring0/ring3 + TSS kernel stack
2. **Syscall gate** — `int 0x80` DPL=3; copy_from/to_user
3. **User VA window** — solo `[USER_BASE, USER_TOP)`
4. **path_ok** — rechaza `..` y rutas absolutas en open
5. **Stack canary** — `__stack_chk_guard` + fail → panic
6. **ASLR** — stack top y brk por proceso
7. **Stack guard page** — página sin mapear bajo el stack user
8. **Strict page fault** — user PF mata el task; no demand-paging libre
9. **Null page** — VA 0 no mapeada en identity map
10. **SMEP/SMAP** — CR4 si CPUID lo anuncia

## Limitaciones conocidas

- **NX** requiere PAE en 32-bit; aún no activado en tablas
- ELF carga en VA fija (no PIE)
- Sin KASLR del kernel image
- Disco SFS no reclama bloques al borrar
