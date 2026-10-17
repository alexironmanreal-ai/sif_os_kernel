# Roadmap SIF Kernel

## Fase 0–3 (hechas)
Boot, IRQs, memoria, scheduler, syscalls, ramfs, ELF, VFS, shell

## Fase 4 — Estructura
- [x] Page directory por proceso + CR3
- [x] Per-process FD table + brk
- [x] mutex basico
- [x] **Filesystem en disco (SFS sobre ATA R/W)**
- [ ] exec asincrono
- [ ] Context switch ring3 completo
- [ ] Preemptive + wait queues mas robustos

## Fase 5 — Apps y GUI
- [ ] Mas programas userspace
- [ ] Framebuffer / GUI minima
