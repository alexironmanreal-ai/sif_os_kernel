# Roadmap SIF Kernel

## Fase 0 — Boot (hecha)
- [x] Multiboot, VGA, GDT, IDT

## Fase 1 — Interrupciones (hecha)
- [x] PIC remap
- [x] IRQ stubs + dispatch
- [x] PIT timer
- [x] Teclado PS/2
- [x] Serial COM1

## Fase 2 — Memoria (hecha)
- [x] Multiboot mmap
- [x] Frame allocator (bitmap)
- [x] Paging (identity map + dynamic tables)
- [x] kmalloc freelist + kfree

## Fase 3 — Procesos y FS (en curso)
- [x] Scheduler cooperativo + round-robin
- [x] Context switch
- [x] Syscalls basicas
- [x] ramfs (create/read/delete/list)
- [x] ATA PIO detect/read
- [x] Shell interactiva

## Fase 4 — Siguiente
- [ ] Paginacion por proceso / user mode
- [ ] ELF loader
- [ ] Syscalls completas (read/write/open/exit)
- [ ] Filesystem en disco (simple FAT o propio)
- [ ] VFS layer
- [ ] Preemptive scheduling mejorado
- [ ] Mutex / sleep / wait queues
