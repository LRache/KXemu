.global main

#define MTIMECMP_BASE 0x02004000

#define MSTATUS_MIE_MASK (1 << 3)

main:
    la t0, m_mode_handle
    csrw mtvec, t0

    li t1, MTIMECMP_BASE
#if __riscv_xlen == 32
    li t0, 10000000
    sw t0, 0(t1)
    sw zero, 4(t1)
#else
    li t0, 10000000
    sd t0, 0(t1)
#endif

    li t0, MSTATUS_MIE_MASK
    csrs mstatus, t0

    li t0, (1 << 7)
    csrs mie, t0

    li s0, 0

.loop:
    wfi
    j .loop

.align 2
m_mode_handle:
    csrr t0, mcause
#if __riscv_xlen == 32
    li t1, 0x80000007
#else
    li t1, 0x8000000000000007
#endif
    bne t0, t1, .fail

    la a0, str1
    call puts
    addi s0, s0, 1
    la a0, str2
    mv a1, s0
    call printf

    li t1, MTIMECMP_BASE
    li t2, 10000000
#if __riscv_xlen == 32
    lw t0, 0(t1)
    add t0, t0, t2
    sw t0, 0(t1)
    sw zero, 4(t1)
#else
    ld t0, 0(t1)
    li t2, 10000000
    add t0, t0, t2
    sd t0, 0(t1)
#endif

    mret

.fail:
    la a0, str0
    call puts

    li a0, 1
    ebreak
    j .

.section .data
str0:
    .string "Should not reach here."
str1:
    .string "Machine Timer interrupt."
str2:
    .string "%ds\n"
