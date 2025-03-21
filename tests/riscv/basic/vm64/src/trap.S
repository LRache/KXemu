.globl user_entry
.globl m_trap_entry

.global m_trap_handler

#if __riscv_xlen == 64
    #define STORE sd
    #define LOAD  ld
    #define XLEN  8
#else
    #define STORE sw
    #define LOAD  lw
    #define XLEN  4
#endif

#define CONTEXT_SIZE 32 * XLEN

.align 2
m_trap_entry:
    // Save stack pointer of the u-mode
    csrrw sp, mscratch, sp

    addi sp, sp, -CONTEXT_SIZE

    STORE x1, (1 * XLEN)(sp)
    STORE x3, (3 * XLEN)(sp)
    STORE x4, (4 * XLEN)(sp)
    STORE x5, (5 * XLEN)(sp)
    STORE x6, (6 * XLEN)(sp)
    STORE x7, (7 * XLEN)(sp)
    STORE x8, (8 * XLEN)(sp)
    STORE x9, (9 * XLEN)(sp)
    STORE x10, (10 * XLEN)(sp)
    STORE x11, (11 * XLEN)(sp)
    STORE x12, (12 * XLEN)(sp)
    STORE x13, (13 * XLEN)(sp)
    STORE x14, (14 * XLEN)(sp)
    STORE x15, (15 * XLEN)(sp)
    STORE x16, (16 * XLEN)(sp)
    STORE x17, (17 * XLEN)(sp)
    STORE x18, (18 * XLEN)(sp)
    STORE x19, (19 * XLEN)(sp)
    STORE x20, (20 * XLEN)(sp)
    STORE x21, (21 * XLEN)(sp)
    STORE x22, (22 * XLEN)(sp)
    STORE x23, (23 * XLEN)(sp)
    STORE x24, (24 * XLEN)(sp)
    STORE x25, (25 * XLEN)(sp)
    STORE x26, (26 * XLEN)(sp)
    STORE x27, (27 * XLEN)(sp)
    STORE x28, (28 * XLEN)(sp)
    STORE x29, (29 * XLEN)(sp)
    STORE x30, (30 * XLEN)(sp)
    STORE x31, (31 * XLEN)(sp)

    mv a0, sp
    call m_trap_handler

    addi sp, sp, CONTEXT_SIZE

.align 2
user_entry:
    // Save stack pointer of the m-mode
    csrw mscratch, sp
    
    mv sp, a0

    LOAD x1, (1 * XLEN)(sp)
    LOAD x3, (3 * XLEN)(sp)
    LOAD x4, (4 * XLEN)(sp)
    LOAD x5, (5 * XLEN)(sp)
    LOAD x6, (6 * XLEN)(sp)
    LOAD x7, (7 * XLEN)(sp)
    LOAD x8, (8 * XLEN)(sp)
    LOAD x9, (9 * XLEN)(sp)
    LOAD x10, (10 * XLEN)(sp)
    LOAD x11, (11 * XLEN)(sp)
    LOAD x12, (12 * XLEN)(sp)
    LOAD x13, (13 * XLEN)(sp)
    LOAD x14, (14 * XLEN)(sp)
    LOAD x15, (15 * XLEN)(sp)
    LOAD x16, (16 * XLEN)(sp)
    LOAD x17, (17 * XLEN)(sp)
    LOAD x18, (18 * XLEN)(sp)
    LOAD x19, (19 * XLEN)(sp)
    LOAD x20, (20 * XLEN)(sp)
    LOAD x21, (21 * XLEN)(sp)
    LOAD x22, (22 * XLEN)(sp)
    LOAD x23, (23 * XLEN)(sp)
    LOAD x24, (24 * XLEN)(sp)
    LOAD x25, (25 * XLEN)(sp)
    LOAD x26, (26 * XLEN)(sp)
    LOAD x27, (27 * XLEN)(sp)
    LOAD x28, (28 * XLEN)(sp)
    LOAD x29, (29 * XLEN)(sp)
    LOAD x30, (30 * XLEN)(sp)
    LOAD x31, (31 * XLEN)(sp)

    // Load the stack pointer of the u-mode
    LOAD sp, (2 * XLEN)(sp)

    mret
