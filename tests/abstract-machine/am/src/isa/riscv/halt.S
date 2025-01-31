.globl __halt

__halt:
#ifdef __QEMU__
    mv a1, a0
    la a0, ebreakMessage
    call printf
    j .
#else
    ebreak
#endif
    j .

.section .data
ebreakMessage:
    .asciz "HALT with code %d.\n"
