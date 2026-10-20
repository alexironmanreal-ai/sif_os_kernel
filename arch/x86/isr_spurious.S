/*
 * Default IDT gate — ignore unexpected vectors.
 * Do NOT call fault_handler (would panic).
 */
.global isr_spurious
.type isr_spurious, @function
isr_spurious:
    iret
.size isr_spurious, . - isr_spurious
