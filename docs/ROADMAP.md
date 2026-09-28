# Roadmap — de kernel base a SO

## Fase 0 — Base (actual)
- [x] Multiboot
- [x] VGA printk
- [x] GDT
- [x] IDT vacía

## Fase 1 — Hardware básico
- [ ] PIC remap
- [ ] IRQ timer (PIT)
- [ ] IRQ teclado
- [ ] Serial debug (COM1)

## Fase 2 — Memoria
- [ ] Mapa físico (Multiboot mmap)
- [ ] Bitmap de frames
- [ ] Paging (4 KB pages)
- [ ] kmalloc / kfree

## Fase 3 — Procesos
- [ ] PCB
- [ ] Context switch
- [ ] Scheduler round-robin
- [ ] User mode (ring3) + TSS

## Fase 4 — Syscalls + userspace
- [ ] Syscall table
- [ ] write/read/exit/fork
- [ ] Primer proceso init (ELF)

## Fase 5 — Disco + FS
- [ ] ATA PIO
- [ ] Filesystem simple
- [ ] exec()
