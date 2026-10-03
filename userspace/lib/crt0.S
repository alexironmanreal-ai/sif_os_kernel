.global _start
.extern main
.extern exit
_start:
    call main
    push %eax
    call exit
1:  hlt
    jmp 1b
