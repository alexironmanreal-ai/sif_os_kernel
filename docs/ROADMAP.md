# Roadmap SIF Kernel

## Fase 0 — Boot (hecha)
- [x] Multiboot, VGA, GDT, IDT

## Fase 1 — Interrupciones (hecha)
- [x] PIC remap, IRQ, PIT, teclado, serial

## Fase 2 — Memoria (hecha)
- [x] PMM, paging, kmalloc

## Fase 3 — Procesos y FS (hecha)
- [x] Scheduler, syscalls, ramfs, ATA, shell, ELF, VFS basica

## Fase 4 — Estructura de procesos
- [x] Page directory por proceso + switch CR3
- [x] exec con address space privado + cleanup
- [x] task: cr3 / is_user / parent / kstack
- [x] **Per-process file descriptor table**
- [x] **Per-process brk (heap)**
- [x] **mutex + wait queue basico**
- [ ] exec asincrono (shell no bloquea)
- [ ] Context switch completo ring3
- [ ] Filesystem en disco (FAT simple)
- [ ] Preemptive scheduling mas robusto

## Fase 5 — Apps y GUI (despues)
- [ ] Mas programas userspace
- [ ] Framebuffer / modo grafico
- [ ] GUI minima
