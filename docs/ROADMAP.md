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

## Fase 3 — Procesos y FS (hecha)
- [x] Scheduler cooperativo + round-robin
- [x] Context switch
- [x] Syscalls basicas (+ getdents)
- [x] ramfs (create/read/delete/list)
- [x] ATA PIO detect/read
- [x] Shell interactiva
- [x] ELF loader + userspace programs
- [x] VFS layer basica (stdin/stdout/fd)

## Fase 4 — Estructura de procesos (en curso)
- [x] Page directory por proceso (clone kernel + user region privada)
- [x] Switch CR3 en schedule
- [x] exec crea address space + libera al salir
- [x] task slots con cr3 / is_user / parent / kstack
- [ ] exec asincrono (proceso corre como task real, shell no bloquea)
- [ ] Context switch completo ring3 (frames de interrupcion)
- [ ] Per-process brk / file descriptor table
- [ ] Filesystem en disco (FAT simple o propio)
- [ ] Preemptive scheduling robusto + mutex / wait queues

## Fase 5 — Apps y GUI (despues)
- [ ] Mas programas userspace (editor, calculator, ...)
- [ ] Framebuffer / modo grafico basico
- [ ] Window manager minimo / GUI toolkit
