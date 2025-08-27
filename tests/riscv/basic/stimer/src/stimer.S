.global main

#define STATUS_SIE_MASK (1 << 1)
#define STATUS_MPP_MASK (3 << 11)
#define STATUS_SPIE_MASK (1 << 5)
#define IE_STIE_MASK (1 << 5)
#define IP_STIP_MASK (1 << 5)

main:
    // Configure Physical Memory Protection to give supervisor and 
    // user mode access to all of physical memory.
    li t0, 0xffffffff
    csrw pmpaddr0, t0
    li t0, 0xf
    csrw pmpcfg0, t0

    la t0, s_mode_handle
    csrw stvec, t0

    // Enable machine timer interrupt
    li t0, STATUS_SIE_MASK
    csrs mstatus, t0
    li t0, IE_STIE_MASK
    csrs mie, t0

    // Set MPP to user mode
    li t0, STATUS_MPP_MASK
    csrc mstatus, t0

    // Delegate interrupts to supervisor mode
    li t0, 0xffff
    csrs mideleg, t0

    // Set supervisor timer interrupt
    csrr t0, time
    li t1, 10000000
    add t0, t0, t1
    csrw stimecmp, t0

    li t0, (1 << 1)
    csrs mcounteren, t0
    li t0, (1 << 63)
    csrs menvcfg, t0

    la t0, m_mode_handle
    csrw mtvec, t0

    // Enter to user mode
    la t0, u_mode
    csrw mepc, t0
    mret

.align 2
u_mode:
    li s0, 0
.loop:
    j .loop

.align 2
s_mode_handle:
    csrr t0, scause
#if __riscv_xlen == 32
    li t1, 0x80000005
#else
    li t1, 0x8000000000000005
#endif
    bne t0, t1, .fail

    la a0, str1
    call puts
    addi s0, s0, 1
    la a0, str2
    mv a1, s0
    call printf

    csrr t0, time
    li t1, 10000000
    add t0, t0, t1
    csrw stimecmp, t0

    li t0, IP_STIP_MASK
    csrc sip, t0

    sret

.fail:
    la a0, str0
    call puts

    li a0, 1
    call __halt

m_mode_handle:
    csrr a1, mcause
    la a0, str3
    call printf
    
    csrr a1, mtval
    la a0, str4
    call printf

    li a0, 1
    call __halt

.section .rodata
str0:
    .string "Should not reach here."
str1:
    .string "Supervisor Timer interrupt."
str2:
    .string "%ds\n"
str3:
    .string "mcause=%x\n"
str4:
    .string "mtval=%x\n"
