/*
 * SIF bootloader stub — early markers before C kernel_main.
 * Called from _start after stack setup; returns to continue boot.
 */
.section .text
.global boot_stub
.type boot_stub, @function
boot_stub:
    push %ebp
    mov %esp, %ebp
    push %eax
    push %ebx

    /* serial COM1: "[stub] ok\r\n" if serial already usable; best-effort */
    mov $0x3F8, %dx
    mov $'[' , %al
    outb %al, %dx
    mov $'s' , %al
    outb %al, %dx
    mov $'t' , %al
    outb %al, %dx
    mov $'u' , %al
    outb %al, %dx
    mov $'b' , %al
    outb %al, %dx
    mov $']' , %al
    outb %al, %dx
    mov $' ' , %al
    outb %al, %dx
    mov $'o' , %al
    outb %al, %dx
    mov $'k' , %al
    outb %al, %dx
    mov $'\r' , %al
    outb %al, %dx
    mov $'\n' , %al
    outb %al, %dx

    pop %ebx
    pop %eax
    pop %ebp
    ret
.size boot_stub, . - boot_stub
