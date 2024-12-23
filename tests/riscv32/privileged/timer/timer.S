.global main

#define MTIMECMP_BASE 0x02004000

#define MSTATUS_MIE_MASK (1 << 3)

main:
    la t0, m_mode_handle
    csrw mtvec, t0

    li t1, MTIMECMP_BASE
    li t0, 1000000000
    sw t0, 0(t1)
    li t0, 0
    sw t0, 4(t1)

    li t0, MSTATUS_MIE_MASK
    csrs mstatus, t0

    li t0, (1 << 7)
    csrs mie, t0

    j .

.align 4   
m_mode_handle:
    csrr t0, mcause
    li t1, 0x80000007
    bne t0, t1, .fail

    la a0, str1
    call puts
    mret

.fail:
    la a0, str0
    call puts

    li a0, 1
    ebreak

.section .data
str0:
    .string "Should not reach here."
str1:
    .string "Machine Timer interrupt."
