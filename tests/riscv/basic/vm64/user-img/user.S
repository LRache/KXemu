#define ADDR 0x2000100a

_entry:
.loop:
    li t0, ADDR
    ld t1, 0(t0)

    li a0, 1
    mv a1, t1
    ecall

    li t1, 0x52CAFFEE
    sd t1, 8(t0)

    li a0, 2
    ecall

    j .loop
