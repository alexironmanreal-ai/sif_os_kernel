/* optional early marker — higher-half safe after paging */
.section .text
.global boot_stub
.type boot_stub, @function
boot_stub:
    ret
.size boot_stub, . - boot_stub
