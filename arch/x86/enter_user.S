.global enter_usermode
.type enter_usermode, @function
enter_usermode:
    cli
    mov $0x23, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov 8(%esp), %eax
    mov 4(%esp), %ecx
    pushl $0x23
    pushl %eax
    pushf
    popl %edx
    orl $0x200, %edx
    pushl %edx
    pushl $0x1B
    pushl %ecx
    iret
