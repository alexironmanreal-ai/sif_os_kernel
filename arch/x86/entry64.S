/*
 * 64-bit entry (reached only after longmode_enter).
 * Assembled inside 32-bit object via .code64.
 */
.section .text
.code64
.global entry64
.type entry64, @function
entry64:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs

    /* use same stack top (identity mapped) */
    mov $stack_top, %rsp
    and $-16, %rsp

    /* serial COM1: "SIF 64-bit long mode\r\n" */
    lea msg64(%rip), %rsi
.sloop:
    movzb (%rsi), %eax
    test %al, %al
    jz .vga
    mov $0x3F8, %dx
1:
    add $5, %dx
    inb %dx, %al
    sub $5, %dx
    test $0x20, %al
    jz 1b
    movzb (%rsi), %eax
    outb %al, %dx
    inc %rsi
    jmp .sloop

.vga:
    /* write banner at VGA text 0xB8000 */
    mov $0xB8000, %rdi
    lea msg64(%rip), %rsi
    mov $0x1F, %ah              /* bright white on blue */
.vloop:
    movb (%rsi), %al
    test %al, %al
    jz .halt
    movw %ax, (%rdi)
    add $2, %rdi
    inc %rsi
    jmp .vloop

.halt:
    cli
1:
    hlt
    jmp 1b

.section .rodata
msg64:
    .asciz "SIF 64-bit long mode OK\r\n"

.extern stack_top
.code32
