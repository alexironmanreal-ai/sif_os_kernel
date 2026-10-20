/*
 * 64-bit entry after longmode_enter.
 * Must assemble with gcc -m32: no RIP-relative / 64-bit ELF relocs.
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

    /* stack: low identity address (phys stack_top may be high after HH;
       for pre-HH builds use a fixed low stack). Use 0x90000 scratch. */
    mov $0x90000, %esp
    mov %rsp, %rbp

    /* COM1 0x3F8: print "SIF64\r\n" without memory operands */
    mov $0x3F8, %dx

    mov $'S', %al
    call .out
    mov $'I', %al
    call .out
    mov $'F', %al
    call .out
    mov $'6', %al
    call .out
    mov $'4', %al
    call .out
    mov $'\r', %al
    call .out
    mov $'\n', %al
    call .out

    /* VGA text cell at 0xB8000 */
    mov $0xB8000, %edi
    mov $0x1F00, %eax
    mov $'S', %al
    mov %ax, (%rdi)
    mov $'I', %al
    mov %ax, 2(%rdi)
    mov $'F', %al
    mov %ax, 4(%rdi)
    mov $'6', %al
    mov %ax, 6(%rdi)
    mov $'4', %al
    mov %ax, 8(%rdi)

    cli
1:
    hlt
    jmp 1b

.out:
    push %ax
    mov $0x3FD, %dx
2:
    inb %dx, %al
    test $0x20, %al
    jz 2b
    mov $0x3F8, %dx
    pop %ax
    outb %al, %dx
    ret

.size entry64, . - entry64
.code32
