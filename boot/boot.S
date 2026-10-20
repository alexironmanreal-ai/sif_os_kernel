/*
 * SIF dual boot entry — Multiboot1 + Multiboot2
 * 32-bit protected mode entry from GRUB/QEMU -kernel
 */
.set MB1_ALIGN,    1<<0
.set MB1_MEMINFO,  1<<1
.set MB1_FLAGS,    MB1_ALIGN | MB1_MEMINFO
.set MB1_MAGIC,    0x1BADB002
.set MB1_CHECKSUM, -(MB1_MAGIC + MB1_FLAGS)

.section .multiboot, "a"
.align 4
mb1_header:
    .long MB1_MAGIC
    .long MB1_FLAGS
    .long MB1_CHECKSUM

.set MB2_MAGIC,          0xE85250D6
.set MB2_ARCH_I386,      0
.set MB2_HEADER_LENGTH,  mb2_header_end - mb2_header_start
.set MB2_CHECKSUM,       -(MB2_MAGIC + MB2_ARCH_I386 + MB2_HEADER_LENGTH)

.align 8
mb2_header_start:
    .long MB2_MAGIC
    .long MB2_ARCH_I386
    .long MB2_HEADER_LENGTH
    .long MB2_CHECKSUM
    .align 8
    .word 0
    .word 0
    .long 8
mb2_header_end:

.section .bss
.align 16
stack_bottom:
    .skip 32768
stack_top:

.align 4096
.global lm_pml4
lm_pml4:
    .skip 4096
.global lm_pdpt
lm_pdpt:
    .skip 4096
.global lm_pd
lm_pd:
    .skip 4096

.section .text
.global _start
.type _start, @function
.extern boot_stub
.extern kernel_main
_start:
    cli
    mov $stack_top, %esp
    and $-16, %esp
    xor %ebp, %ebp
    cld

    /* preserve Multiboot EAX/EBX across stub */
    push %eax
    push %ebx
    call boot_stub
    pop %ebx
    pop %eax

    push %ebx
    push %eax
    call kernel_main

    cli
.hang:
    hlt
    jmp .hang
.size _start, . - _start
