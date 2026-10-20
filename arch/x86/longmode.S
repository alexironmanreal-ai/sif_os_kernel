/*
 * Enter IA-32e long mode from 32-bit protected mode.
 * Identity-maps first 1 GiB with 2 MiB pages, then jumps to entry64.
 */
.section .data
.align 16
gdt64:
    .quad 0
    /* 0x08: kernel 64-bit code */
    .quad 0x00AF9A000000FFFF
    /* 0x10: kernel data */
    .quad 0x00AF92000000FFFF
gdt64_end:

gdt64_ptr:
    .word gdt64_end - gdt64 - 1
    .long gdt64

.section .text
.global longmode_enter
.type longmode_enter, @function
longmode_enter:
    cli

    /* clear page tables */
    mov $lm_pml4, %edi
    xor %eax, %eax
    mov $3072, %ecx          /* 3 * 4096 / 4 */
    rep stosl

    /*
     * PML4[0] -> PDPT
     * PDPT[0] -> PD
     * PD[i]   -> 2MiB page i (0..511)  => 1 GiB
     */
    mov $lm_pdpt, %eax
    or $0x03, %eax           /* present + rw */
    mov %eax, lm_pml4

    mov $lm_pd, %eax
    or $0x03, %eax
    mov %eax, lm_pdpt

    mov $lm_pd, %edi
    xor %eax, %eax
    mov $0x83, %eax          /* present + rw + PS(2MiB) */
    mov $512, %ecx
.fill_pd:
    mov %eax, (%edi)
    add $0x200000, %eax      /* next 2MiB */
    add $8, %edi
    loop fills_pd

    /* CR4.PAE */
    mov %cr4, %eax
    or $0x20, %eax
    mov %eax, %cr4

    /* CR3 = PML4 */
    mov $lm_pml4, %eax
    mov %eax, %cr3

    /* EFER.LME */
    mov $0xC0000080, %ecx
    rdmsr
    or $0x100, %eax
    wrmsr

    /* CR0.PG */
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    lgdt gdt64_ptr
    ljmp $0x08, $entry64

.size longmode_enter, . - longmode_enter

/* symbols from boot.S BSS */
.extern lm_pml4
.extern lm_pdpt
.extern lm_pd
