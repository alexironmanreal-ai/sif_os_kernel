# SIF Kernel 3.0.0

Kernel propio (no Linux / no Windows). Consola serial + VGA, userland ELF,
red NE2000, FS en RAM + disco SFS, aislamiento ring3, SMEP/SMAP, ASLR.

## Build & run

```bash
git fetch origin && git reset --hard origin/main
make clean && make
make console          # terminal Ubuntu = consola del kernel (Ctrl-A X salir)
# o:
make run              # ventana VGA + serial en esta terminal
```

Requisitos: `gcc-multilib`, `qemu-system-i386`, `python3`, `make`.

## Comandos de consola

| Grupo | Comandos |
|-------|----------|
| Ayuda | `help` `aid` `version` `info` `cpu` |
| FS RAM | `ls` `cat` `touch` `rm` `write` `cp` |
| Disco | `mkfs` `mount` `sync` `dls` `dcat` `dwrite` |
| Procesos | `spawn` `pulse` `kill` |
| Red | `net` `arp` `ping` `dns` `http` |
| Sistema | `date` `uptime` `dmesg` `vault` `pci` `devs` `sec` |
| Historial | `hist` `!!` `!n` |
| Otros | `echo` `clear` `reboot` |

## Arquitectura (resumen)

- x86 32-bit Multiboot, higher-half VMA `0xC0000000`
- GDT ring0/ring3 + TSS, IDT 256, PIC, PIT 100 Hz
- Paging identity 0–16 MiB + HH; **página 0 desmapeada**
- Tasks cooperativos + quantum; user ELF + syscalls `int 0x80`
- Ramfs + SFS en ATA (`disk.img`)
- Stack NE2000: eth → ARP/IPv4/ICMP/UDP/TCP/DNS/HTTP

## Seguridad

- uaccess / path_ok, stack canary, ASLR stack/brk
- Guard page bajo stack user
- SMEP/SMAP si el CPU lo soporta
- Page-fault user → mata el proceso (no auto-map)

Ver `docs/SECURITY.md` y `docs/ROADMAP.md`.
