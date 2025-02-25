.globl __halt

__halt:
#ifdef __QEMU__
    mv a2, a0
    csrr a1, mhartid
    la a0, ebreakMessage
    call printf
    j .
#else
    ebreak
#endif
    j .

.section .data
ebreakMessage:
    .asciz "Core%d HALT with code %d.\n"
