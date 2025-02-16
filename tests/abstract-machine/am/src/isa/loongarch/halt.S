.globl __halt

__halt:
#ifdef __QEMU__
    move $a1, $a0
    la $a0, ebreakMessage
    bl printf
    b .
#else
    break 0
#endif
    b .

.section .data
ebreakMessage:
    .asciz "HALT with code %d.\n"
