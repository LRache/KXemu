.global main
.global puts

main:
    li t0, 0xffffffff
    csrw pmpaddr0, t0
    li t0, 0xf
    csrw pmpcfg0, t0

    // set the mstatues.mpp to S mode
    li t0, 3 << 11
    csrc mstatus, t0
    li t0, 1 << 11
    csrs mstatus, t0
    
    // set the mepc to smode_entry
    la t0, s_mode_entry
    csrw mepc, t0

    la t0, m_mode_handle
    csrw mtvec, t0

    // set the medeleg to 0xffff
    li t0, 0xffff
    csrw medeleg, t0
    
    mret

m_mode_handle:
    // Should not reach here
    la a0, str0
    call puts
    li a0, 3
    ebreak

s_mode_entry:
    la a0, str1
    call puts

    // set the mstatus.mpp to U mode
    li t0, 1 << 8
    csrc sstatus, t0

    la t0, s_mode_handle
    csrw stvec, t0

    la t0, u_mode_entry
    csrw sepc, t0

    sret

s_mode_handle:
    mv s1, a0
    la a0, str2
    call puts
    mv a0, s1

    li t1, 0x123
    bne a0, t1, .fail
    la a0, str4
    call puts

    csrr t0, sepc
    addi t0, t0, 4
    csrw sepc, t0
    sret

.fail:
    la a0, str0
    call puts
    
    li a0, 2
    ebreak

u_mode_entry:
    la a0, str3
    call puts

    li a0, 0x123
    ecall
    li a0, 0
    ebreak

.section .data
str0:
    .string "Should not reach here!"
str1:
    .string "Enter to S-mode"
str2:
    .string "Enter to U-mode"
str3:
    .string "Call from U-mode"
str4:
    .string "U-mode ECALL"
