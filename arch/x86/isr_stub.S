/* IRQ stubs — irq number queda en 36(%esp) tras pusha+ds */
.macro IRQ_STUB num
.global irq\num
.type irq\num, @function
irq\num:
    cli
    pushl $0
    pushl $\num
    jmp irq_common_stub
.endm

IRQ_STUB 0
IRQ_STUB 1
IRQ_STUB 2
IRQ_STUB 3
IRQ_STUB 4
IRQ_STUB 5
IRQ_STUB 6
IRQ_STUB 7
IRQ_STUB 8
IRQ_STUB 9
IRQ_STUB 10
IRQ_STUB 11
IRQ_STUB 12
IRQ_STUB 13
IRQ_STUB 14
IRQ_STUB 15

.extern irq_dispatch
.global irq_common_stub
irq_common_stub:
    pusha
    mov %ds, %ax
    push %eax
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    movl 36(%esp), %eax
    pushl %eax
    call irq_dispatch
    addl $4, %esp
    pop %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    popa
    addl $8, %esp
    iret
