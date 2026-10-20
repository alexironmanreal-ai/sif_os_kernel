/*
 * SIF linker script — higher-half kernel
 *
 * Low (identity):
 *   1MiB: Multiboot header + bootstrap (_start)  → ELF entry is PHYSICAL
 *
 * High (VMA):
 *   0xC0000000 + phys: .text .rodata .data .bss
 *
 * Boot: enable paging (ident + HH), jump to higher-half C kernel.
 */

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

KERNEL_VMA = 0xC0000000;

SECTIONS
{
    /* ========== LOW: physical load @ 1 MiB ========== */
    . = 1M;

    .multiboot :
    {
        KEEP(*(.multiboot))
    }

    .bootstrap : ALIGN(4K)
    {
        *(.bootstrap)
        *(.bootstrap.*)
    }

    . = ALIGN(4K);
    _low_end = .;              /* first free phys after bootstrap */
    kernel_phys_start = 1M;

    /* ========== HIGH: virtual kernel image ========== */
    . = KERNEL_VMA + _low_end;

    kernel_start = .;

    .text : AT(ADDR(.text) - KERNEL_VMA)
    {
        *(.text.hh)
        *(.text .text.*)
    }

    .rodata : AT(ADDR(.rodata) - KERNEL_VMA)
    {
        *(.rodata .rodata.*)
    }

    .data : AT(ADDR(.data) - KERNEL_VMA)
    {
        *(.data .data.*)
    }

    .bss : AT(ADDR(.bss) - KERNEL_VMA)
    {
        *(COMMON)
        *(.bss .bss.*)
    }

    . = ALIGN(4K);
    kernel_end = .;
    kernel_phys_end = . - KERNEL_VMA;

    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.comment)
        *(.eh_frame)
    }
}
