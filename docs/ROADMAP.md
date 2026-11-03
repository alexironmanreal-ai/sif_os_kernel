# SIF Roadmap

## Hecho (3.0)
- [x] Boot Multiboot + consola serial/VGA
- [x] GDT/IDT/PIC/PIT/teclado
- [x] Paging + PMM + kmalloc
- [x] Scheduler + userland ELF + syscalls
- [x] Ramfs + SFS disco (mkfs/mount)
- [x] Red NE2000 + ICMP/DNS/HTTP toy
- [x] SMEP/SMAP, canary, ASLR, null-page
- [x] Historial de comandos

## Próximo
- [ ] PAE + bit NX en PTEs
- [ ] ELF PIE (ASLR de código)
- [ ] Preemption real en kernel paths largos
- [ ] TCP más robusto / múltiples sockets
- [ ] VFS unificado ramfs+diskfs
- [ ] GUI opcional (modo gráfico separado)

## Largo plazo
- [ ] SMP
- [ ] 64-bit kernel path completo
- [ ] Drivers virtio
