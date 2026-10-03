.global _start
.extern main
.extern exit
_start:
    mov (%esp), %eax
    lea 4(%esp), %ecx
    push %ecx
    push %eax
    call main
    add $8, %esp
    push %eax
    call exit
1:  hlt
    jmp 1b
