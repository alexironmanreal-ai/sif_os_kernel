# SIF Kernel — Base de sistema operativo (x86)

Esto **no** es una app de empresa. Es un **kernel mínimo booteable** sobre el que podés construir tu propio SO.

## Qué incluye

- Boot Multiboot (GRUB / QEMU `-kernel`)
- Entrada `_start` + stack
- VGA text + `printk`
- GDT (ring0 code/data)
- IDT (tabla de 256 entradas, base para IRQs)

## Build (Linux / WSL)

```bash
sudo apt install build-essential gcc-multilib qemu-system-x86
make
make run    # abre QEMU
```

## Windows

Usá **WSL2 (Ubuntu)**:

```bash
sudo apt update && sudo apt install build-essential gcc-multilib qemu-system-x86
cd /mnt/c/Users/alex/sif_os_kernel
make && make run
```

## Estructura

```
boot/        Multiboot + entry
arch/x86/    GDT, IDT
kernel/      kernel_main
drivers/     VGA
lib/         printk
include/     headers
```

## Próximos pasos (vos construís arriba)

1. IRQs (timer, teclado)
2. Paging + kmalloc
3. Procesos + scheduler
4. Syscalls + userspace
5. Disco + filesystem

Ver `docs/ROADMAP.md`.
