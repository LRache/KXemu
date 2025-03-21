.global main
.global puts

main:
    // configure Physical Memory Protection to give supervisor mode
    // access to all of physical memory.
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
    
    mret

.align 2
m_mode_handle:
    csrr t0, mepc
    addi t0, t0, 4
    csrw mepc, t0
    
    addi sp, sp, -4
    sw ra, 0(sp)
    la a0, str2
    call puts
    lw ra, 0(sp)
    
    mret

.align 2
s_mode_entry:
    la a0, str1
    call puts
    ecall
    la a0, str3
    call puts

    mv a0, zero
    call halt

.section .data
str1:
    .string "Enter to S-mode"
str2:
    .string "Enter to M-mode"
str3:
    .string "Return to S-mode"
