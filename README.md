# SIF Kernel — Base de SO x86 (Fase 1)

Kernel **booteable** Multiboot con interrupciones reales.

## Incluye (Fase 1)

- PIC 8259 remapeado (IRQ0-15 → int 0x20-0x2F)
- Stubs IRQ en ASM + dispatch en C
- PIT timer 100 Hz (IRQ0)
- Teclado PS/2 (IRQ1) con buffer y eco
- Serial COM1 (debug)
- GDT + IDT + VGA printk

## Build (Linux / WSL)

```bash
sudo apt install build-essential gcc-multilib qemu-system-x86
make clean && make
make run
```

En QEMU: ticks cada ~5s y teclado funcional.

## Windows

Usá **WSL2 Ubuntu** con los mismos comandos.

Repo: https://github.com/alexironmanreal-ai/sif_os_kernel
