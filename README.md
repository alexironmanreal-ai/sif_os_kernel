# SIF Kernel v0.6

Kernel x86 Multiboot de base. Construí tu sistema operativo encima.

## Qué hay

- **Boot**: Multiboot + GDT + IDT + PIC
- **Drivers**: VGA, serial COM1, PIT timer, teclado PS/2, ATA PIO
- **Memoria**: PMM (bitmap), paging, **kmalloc freelist real + kfree**
- **Tareas**: scheduler cooperativo, context switch, demo workers
- **FS**: ramfs con create / read / delete / list
- **Shell**: siempre visible, comandos extendidos

## Comandos de la shell

```
help ps mem heap ticks yield demo clear
echo hola
ls
cat readme.txt
write foo.txt contenido de prueba
rm foo.txt
disk
version
reboot
```

## Compilar y correr

```bash
make clean && make && make run
```

Requiere: `gcc` (multilib i386), `ld`, `qemu-system-i386`.

## Estructura

```
arch/x86/   GDT, IDT, IRQ, syscalls, switch
boot/       Multiboot stub
drivers/    VGA, serial, timer, keyboard, ATA
mm/         PMM, paging, kmalloc
kernel/     main, task, shell, fs
lib/        printk, string
include/    headers
```

## Roadmap

Ver `docs/ROADMAP.md`. Fase 2 (memoria) completada. Fase 3 en curso.
