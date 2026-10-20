/*
 * SIF higher-half linker script
 *
 * Virtual (VMA):  0xC0100000 ...
 * Physical (LMA): 0x00100000 ...  (Multiboot load address)
 *
 * Boot code enables paging (identity + higher-half), then jumps to VMA.
 */

ENTRY(_start)

KERNEL_VMA = 0xC0000000;

SECTIONS
{
    . = KERNEL_VMA + 1M;

    kernel_start = .;
    kernel_phys_start = . - KERNEL_VMA;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA)
    {
        *(.multiboot)
        *(.bootstrap)
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA)
    {
        *(.rodata)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA)
    {
        *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA)
    {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4K);
    kernel_end = .;
    kernel_phys_end = . - KERNEL_VMA;
}
