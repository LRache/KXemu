.global main
.global puts

main:
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

s_mode_entry:
    la a0, str1
    call puts
    ecall
    la a0, str3
    call puts

    li a0, 0
    ebreak

.section .data
str1:
    .string "Enter to S-mode"
str2:
    .string "Enter to M-mode"
str3:
    .string "Return to S-mode"
