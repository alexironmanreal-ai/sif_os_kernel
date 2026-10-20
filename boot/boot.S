/*
 * SIF Multiboot1 entry
 * GRUB sets: EAX = 0x2BADB002, EBX = pointer to multiboot_info
 */
.set ALIGN,    1<<0          /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1          /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot, "a"
.align 4
multiboot_header:
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

.section .bss
.align 16
stack_bottom:
    .skip 32768              /* 32 KiB stack */
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    /* interrupts off until IDT ready */
    cli

    /* stack: 16-byte aligned for SysV ABI */
    mov $stack_top, %esp
    and $-16, %esp
    xor %ebp, %ebp

    /* clear direction flag */
    cld

    /*
     * Multiboot leaves:
     *   EAX = magic 0x2BADB002
     *   EBX = phys addr of multiboot_info
     * Push right-to-left for cdecl: magic first arg, then mbi.
     */
    push %ebx                /* arg2: multiboot_info * */
    push %eax                /* arg1: magic */
    call kernel_main

    /* should never return */
    cli
.hang:
    hlt
    jmp .hang
.size _start, . - _start
