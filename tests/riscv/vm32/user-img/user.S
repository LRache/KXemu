#define ADDR 0x2000100a

_entry:
    li t0, ADDR
    lw t1, 0(t0)

    li a0, 1
    mv a1, t1
    ecall

    li t1, 0x52CAFFEE
    sw t1, 4(t0)

    li a0, 2
    ecall

    li t0, ADDR
    lw t1, 0(t0)

    li a0, 1
    mv a1, t1
    ecall

    li t1, 0x52CAFFEE
    sw t1, 4(t0)

    li a0, 2
    ecall

    j .
