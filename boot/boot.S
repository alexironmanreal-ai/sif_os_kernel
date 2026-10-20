/*
 * Higher-half Multiboot entry
 * Linked at VMA 0xC01xxxxx, executed first at LMA 0x001xxxxx
 */
.set KERNEL_VMA, 0xC0000000

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
.set MB2_HEADER_LENGTH,  mb2_end - mb2_start
.set MB2_CHECKSUM,       -(MB2_MAGIC + MB2_ARCH_I386 + MB2_HEADER_LENGTH)

.align 8
mb2_start:
    .long MB2_MAGIC
    .long MB2_ARCH_I386
    .long MB2_HEADER_LENGTH
    .long MB2_CHECKSUM
    .align 8
    .word 0
    .word 0
    .long 8
mb2_end:

/* early page tables — must be accessible via phys before HH jump */
.section .bss
.align 4096
boot_pd:
    .skip 4096
boot_pt0:
    .skip 4096
boot_pt_hh:
    .skip 4096

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

.section .bootstrap, "ax"
.global _start
.type _start, @function
_start:
    cli
    cld

    /*
     * Running at physical ~1MiB while symbols are VMA.
     * phys(sym) = sym - KERNEL_VMA
     */
    mov $stack_top - KERNEL_VMA, %esp
    and $-16, %esp
    xor %ebp, %ebp

    /* save Multiboot regs */
    mov %eax, %esi
    mov %ebx, %edi

    /* clear PD / PTs */
    mov $boot_pd - KERNEL_VMA, %eax
    mov $1024, %ecx
    xor %edx, %edx
1:
    mov %edx, (%eax)
    add $4, %eax
    loop 1b

    mov $boot_pt0 - KERNEL_VMA, %eax
    mov $1024, %ecx
2:
    mov %edx, (%eax)
    add $4, %eax
    loop 2b

    mov $boot_pt_hh - KERNEL_VMA, %eax
    mov $1024, %ecx
3:
    mov %edx, (%eax)
    add $4, %eax
    loop 3b

    /*
     * Identity map 0-4MiB: PT0[i] = i*4K | PRESENT|WRITE
     * HH map 0xC0000000-0xC0400000 -> 0-4MiB using same frames
     *   PDE[0]    -> boot_pt0
     *   PDE[768]  -> boot_pt_hh  (768 = 0xC0000000 >> 22)
     */
    mov $boot_pt0 - KERNEL_VMA, %eax
    xor %ecx, %ecx
fill_pt0:
    mov %ecx, %edx
    shl $12, %edx
    or $0x3, %edx
    mov %edx, (%eax,%ecx,4)
    inc %ecx
    cmp $1024, %ecx
    jb fill_pt0

    mov $boot_pt_hh - KERNEL_VMA, %eax
    xor %ecx, %ecx
fill_pth:
    mov %ecx, %edx
    shl $12, %edx
    or $0x3, %edx
    mov %edx, (%eax,%ecx,4)
    inc %ecx
    cmp $1024, %ecx
    jb fill_pth

    mov $boot_pd - KERNEL_VMA, %eax
    mov $boot_pt0 - KERNEL_VMA, %edx
    or $0x3, %edx
    mov %edx, (%eax)              /* PDE 0 */

    mov $boot_pt_hh - KERNEL_VMA, %edx
    or $0x3, %edx
    mov %edx, 768*4(%eax)         /* PDE 768 = 0xC0000000 */

    /* CR3 = phys PD */
    mov $boot_pd - KERNEL_VMA, %eax
    mov %eax, %cr3

    /* enable paging */
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    /* absolute jump into higher-half */
    lea hh_entry, %eax
    jmp *%eax

.size _start, . - _start

.section .text
hh_entry:
    /* now PC is in higher-half; use virtual stack */
    mov $stack_top, %esp
    and $-16, %esp
    xor %ebp, %ebp

    /* Multiboot: restore magic/info (still physical pointer in EDI) */
    push %edi                     /* mbi phys — low identity still works */
    push %esi                     /* magic */
    call kernel_main

    cli
.hang:
    hlt
    jmp .hang
