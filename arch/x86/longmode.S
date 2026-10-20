/*
 * Enter IA-32e long mode from 32-bit protected mode.
 */
.section .data
.align 16
gdt64:
    .quad 0
    .quad 0x00AF9A000000FFFF
    .quad 0x00AF92000000FFFF
gdt64_end:

gdt64_ptr:
    .word gdt64_end - gdt64 - 1
    .long gdt64

.section .text
.global longmode_enter
.type longmode_enter, @function
.extern lm_pml4
.extern lm_pdpt
.extern lm_pd
.extern entry64

longmode_enter:
    cli

    /* clear 3 pages */
    mov $lm_pml4, %edi
    xor %eax, %eax
    mov $3072, %ecx
    rep stosl

    mov $lm_pdpt, %eax
    or $0x03, %eax
    mov %eax, lm_pml4

    mov $lm_pd, %eax
    or $0x03, %eax
    mov %eax, lm_pdpt

    mov $lm_pd, %edi
    mov $0x83, %eax
    mov $512, %ecx
.fill_pd:
    mov %eax, (%edi)
    add $0x200000, %eax
    add $8, %edi
    loop fills_pd

    mov %cr4, %eax
    or $0x20, %eax
    mov %eax, %cr4

    mov $lm_pml4, %eax
    mov %eax, %cr3

    mov $0xC0000080, %ecx
    rdmsr
    or $0x100, %eax
    wrmsr

    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    lgdt gdt64_ptr
    ljmp $0x08, $entry64

.size longmode_enter, . - longmode_enter
