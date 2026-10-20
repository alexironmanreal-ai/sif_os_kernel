/*
 * Low Multiboot entry @ phys 1MiB → paging → higher-half kernel_main
 */
.set KERNEL_VMA, 0xC0000000

.set MB1_ALIGN,    1<<0
.set MB1_MEMINFO,  1<<1
.set MB1_FLAGS,    MB1_ALIGN | MB1_MEMINFO
.set MB1_MAGIC,    0x1BADB002
.set MB1_CHECKSUM, -(MB1_MAGIC + MB1_FLAGS)

.section .multiboot, "a"
.align 4
    .long MB1_MAGIC
    .long MB1_FLAGS
    .long MB1_CHECKSUM

.set MB2_MAGIC, 0xE85250D6
.set MB2_ARCH,  0
.set MB2_LEN,   mb2_end - mb2_start
.set MB2_CHK,   -(MB2_MAGIC + MB2_ARCH + MB2_LEN)
.align 8
mb2_start:
    .long MB2_MAGIC
    .long MB2_ARCH
    .long MB2_LEN
    .long MB2_CHK
    .align 8
    .word 0
    .word 0
    .long 8
mb2_end:

.section .bootstrap, "aw", @nobits
.align 4096
boot_pd:
    .skip 4096
boot_pt:
    .skip 4096 * 4
boot_pt_hh:
    .skip 4096 * 4
.align 16
early_stack:
    .skip 16384
early_stack_top:
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
    mov $early_stack_top, %esp
    and $-16, %esp

    /* Multiboot: EAX=magic EBX=info */
    push %eax
    push %ebx

    /* clear page structures */
    mov $boot_pd, %edi
    xor %eax, %eax
    mov $(4096 * 9) / 4, %ecx
    rep stosl

    /* identity PTs: 0 .. 16MiB */
    mov $boot_pt, %edi
    mov $0x3, %eax
    mov $(1024 * 4), %ecx
1:
    mov %eax, (%edi)
    add $0x1000, %eax
    add $4, %edi
    loop 1b

    /* HH PTs: same physical pages */
    mov $boot_pt_hh, %edi
    mov $0x3, %eax
    mov $(1024 * 4), %ecx
2:
    mov %eax, (%edi)
    add $0x1000, %eax
    add $4, %edi
    loop 2b

    /* PDEs 0-3 = identity tables */
    mov $boot_pd, %edi
    mov $boot_pt, %eax
    or $0x3, %eax
    mov %eax, 0(%edi)
    add $4096, %eax
    mov %eax, 4(%edi)
    add $4096, %eax
    mov %eax, 8(%edi)
    add $4096, %eax
    mov %eax, 12(%edi)

    /* PDEs 768-771 = HH tables (0xC0000000) */
    mov $boot_pt_hh, %eax
    or $0x3, %eax
    mov %eax, (768 * 4)(%edi)
    add $4096, %eax
    mov %eax, (769 * 4)(%edi)
    add $4096, %eax
    mov %eax, (770 * 4)(%edi)
    add $4096, %eax
    mov %eax, (771 * 4)(%edi)

    mov $boot_pd, %eax
    mov %eax, %cr3

    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    /* near jump to high virtual address of hh_entry */
    lea hh_entry, %eax
    jmp *%eax

.size _start, . - _start

.section .text
.extern kernel_main
.global hh_entry
hh_entry:
    mov $early_stack_top, %esp
    and $-16, %esp

    pop %ebx                 /* mbi */
    pop %eax                 /* magic */

    push %ebx
    push %eax
    call kernel_main

    cli
3:
    hlt
    jmp 3b
