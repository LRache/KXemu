.globl _start

_start:
    la sp, _stack_pointer # load stack pointer
    call __init
    ebreak
