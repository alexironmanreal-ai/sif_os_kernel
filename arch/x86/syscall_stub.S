.extern syscall_dispatch
.global isr_syscall
.type isr_syscall, @function
isr_syscall:
    pushl %edx
    pushl %ecx
    pushl %ebx
    pushl %eax
    call syscall_dispatch
    addl $16, %esp
    iret
