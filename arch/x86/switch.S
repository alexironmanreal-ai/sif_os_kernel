.global context_switch
.type context_switch, @function
context_switch:
    movl 4(%esp), %eax
    movl 8(%esp), %ecx
    movl %edi, 0(%eax)
    movl %esi, 4(%eax)
    movl %ebp, 8(%eax)
    movl %esp, 12(%eax)
    movl %ebx, 16(%eax)
    movl (%esp), %edx
    movl %edx, 32(%eax)
    movl 0(%ecx), %edi
    movl 4(%ecx), %esi
    movl 8(%ecx), %ebp
    movl 12(%ecx), %esp
    movl 16(%ecx), %ebx
    movl 32(%ecx), %edx
    pushl %edx
    ret
