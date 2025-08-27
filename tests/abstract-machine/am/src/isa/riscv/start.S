.section entry, "ax"
.globl _start

_start:
    # set stack pointer
    la t0, __stack_pointer
    csrr a0, mhartid
    li t1, 0x4000
    mul a0, a0, t1
    add sp, t0, a0
    
    call __run_main

    j .
