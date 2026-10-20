/* Default handler for unassigned IDT vectors */
.global isr_spurious
.type isr_spurious, @function
isr_spurious:
    cli
    pushl $0
    pushl $0xFF
    jmp isr_common
.size isr_spurious, . - isr_spurious
