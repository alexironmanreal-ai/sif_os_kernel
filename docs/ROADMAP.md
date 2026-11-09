# SIF Roadmap

Ver también `docs/STATUS.md` (métricas honestas).

## Hecho
- [x] Boot Multiboot + consola
- [x] GDT/IDT/PIC/PIT/kbd/serial
- [x] Paging + PMM + kmalloc
- [x] Scheduler + user ELF + syscalls
- [x] Ramfs + SFS + VFS unificado
- [x] Red NE2000 toy stack
- [x] SMEP/SMAP, canary, ASLR stack/brk, null page
- [x] Preemption: timer → task_on_timer (3.1)

## Siguiente (prioridad)
- [ ] PAE + NX en PTEs
- [ ] ELF PIE
- [ ] stat/mmap/pipe syscalls
- [ ] TCP con estado mínimo robusto
- [ ] Stress tests + smoke ampliado

## Luego
- [ ] Virtio / mejor block layer
- [ ] SMP
- [ ] 64-bit kernel path real
- [ ] GUI opcional
