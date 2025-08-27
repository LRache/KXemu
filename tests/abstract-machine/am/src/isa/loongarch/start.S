.section entry, "ax"
.globl _start

_start:
    # set stack pointer
    la.local $sp, __stack_pointer
    
    bl __run_main

    b .
