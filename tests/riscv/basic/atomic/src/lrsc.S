.global main

main:
    la t0, data
    lr.d t1, 0(t0)   // Load reserved
    li t2, 0x12345678
    sc.d a0, t2, 0(t0)   // Store conditional

    ebreak

.section .data
data:
    .align 8
    .word 0x00caffee  // Reserved word
    .word 0xffffffff  // Reserved word
