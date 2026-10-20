/*
 * Low bootstrap (identity @ ~1MiB) → enable paging → higher-half
 */
.set KERNEL_VMA, 0xC0000000

/* ---- Multiboot1 (must be in first 8K of OS image) ---- */
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

/* Multiboot2 header */
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

/* ---- identity-mapped early data ---- */
.section .bootstrap, "aw", @nobits
.align 4096
boot_pd:
    .skip 4096
/* 4 page tables = 16 MiB identity */
boot_pt:
    .skip 4096 * 4
/* 4 page tables for HH 0xC0000000 */
boot_pt_hh:
    .skip 4096 * 4

.align 16
early_stack:
    .skip 16384
early_stack_top:

/* longmode scratch (also low for simplicity) */
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

    /* early stack — physical / identity */
    mov $early_stack_top, %esp
    and $-16, %esp
    xor %ebp, %ebp

    mov %eax, %esi          /* multiboot magic */
    mov %ebx, %edi          /* multiboot info  */

    /* zero PD + PTs (5 + 4 pages) */
    mov $boot_pd, %eax
    mov $(4096 * 9) / 4, %ecx
    xor %edx, %edx
    mov %eax, %edi
    rep stosl
    mov %ebx, %edi          /* restore mbi */
    /* careful: edi was mbi — restore */
    mov %ebx, %edi

    /* rebuild: ESI=magic EDI=mbi still? we clobbered edi with stosl */
    /* Save mbi on stack */
    /* Actually at start we had mov %ebx,%edi then stosl used edi. Fix: */

    /* --- restart clean: use stack-saved mbi --- */
    /* Re-read: we need to fix the zeroing to not lose EDI */

.size _start, 0  /* placeholder — full code below */

/* Full correct _start */
.global _start
_start:
    cli
    cld
    mov $early_stack_top, %esp
    and $-16, %esp

    push %eax               /* magic */
    push %ebx               /* mbi   */

    /* clear boot_pd and tables */
    mov $boot_pd, %edi
    xor %eax, %eax
    mov $(4096 * 9) / 4, %ecx
    rep stosl

    /* fill 4 identity PTs: phys 0..16MiB */
    mov $boot_pt, %edi
    xor %eax, %eax
    or $0x3, %eax           /* present|rw */
    mov $1024 * 4, %ecx
1:
    mov %eax, (%edi)
    add $0x1000, %eax
    add $4, %edi
    loop 1b

    /* fill 4 HH PTs: same phys mapping */
    mov $boot_pt_hh, %edi
    xor %eax, %eax
    or $0x3, %eax
    mov $1024 * 4, %ecx
2:
    mov %eax, (%edi)
    add $0x1000, %eax
    add $4, %edi
    loop 2b

    /* PDE[0..3] identity, PDE[768..771] HH */
    mov $boot_pd, %edi
    mov $boot_pt, %eax
    or $0x3, %eax
    mov $4, %ecx
    xor %edx, %edx
3:
    mov %eax, (%edi,%edx,4)
    add $4096, %eax
    inc %edx
    loop 3b

    mov $boot_pt_hh, %eax
    or $0x3, %eax
    mov $4, %ecx
    mov $768, %edx
4:
    mov %eax, (%edi,%edx,4)
    add $4096, %eax
    inc %edx
    loop 4b

    mov $boot_pd, %eax
    mov %eax, %cr3

    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    /* jump to higher-half */
    ljmp $0x08, $hh_entry   /* needs GDT? still using multiboot GDT flat */
    /* Multiboot flat GDT: code seg 0x08 usually — actually GRUB leaves us
       with segments base 0 limit 4G. Absolute jump by address: */

    /* Use indirect jump with high address of hh_entry */
    /* If GDT code is 32-bit flat, jmp *%eax works for near jump in same cs */

    lea hh_entry, %eax
    jmp *%eax

/* higher-half continues in .text */
.section .text
.extern kernel_main
hh_entry:
    /* reload stack to high BSS stack — use early stack still mapped via HH
       early_stack is only in low VMA; HH maps phys so low VA still works */
    mov $early_stack_top, %esp
    and $-16, %esp

    pop %ebx                /* mbi   */
    pop %eax                /* magic */

    push %ebx
    push %eax
    call kernel_main

    cli
5:
    hlt
    jmp 5b
