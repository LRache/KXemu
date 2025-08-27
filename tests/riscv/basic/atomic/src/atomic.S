.section .data
.align 4
val_swap:   .word 42
val_add:    .word 1
val_and:    .word 0b1100
val_or:     .word 0b0011
val_xor:    .word 0b0101
val_min:    .word -3
val_max:    .word -3
val_minu:   .word 0xffffffff  # 4294967295
val_maxu:   .word 1
val_add64:  .dword 100
val_lrsc32: .word 123
val_lrsc64: .word 1000

.section .text
.globl main
main:
    # -------- amoswap.w --------
    la t0, val_swap
    li t1, 99
    amoswap.w t2, t1, (t0)    # t2 = 42
    li t3, 42
    bne t2, t3, fail
    lw t4, 0(t0)
    li t5, 99
    bne t4, t5, fail

    # -------- amoadd.w --------
    la t0, val_add
    li t1, 2
    amoadd.w t2, t1, (t0)     # val = 3, t2 = 1
    li t3, 1
    bne t2, t3, fail
    lw t4, 0(t0)
    li t5, 3
    bne t4, t5, fail

    # -------- amoand.w --------
    la t0, val_and
    li t1, 0b1010
    amoand.w t2, t1, (t0)     # val = 0b1000
    li t3, 0b1100
    bne t2, t3, fail
    lw t4, 0(t0)
    li t5, 0b1000
    bne t4, t5, fail

    # -------- amoor.w --------
    la t0, val_or
    li t1, 0b1100
    amoor.w t2, t1, (t0)      # val = 0b1111
    li t3, 0b0011
    bne t2, t3, fail
    lw t4, 0(t0)
    li t5, 0b1111
    bne t4, t5, fail

    # -------- amoxor.w --------
    la t0, val_xor
    li t1, 0b0011
    amoxor.w t2, t1, (t0)     # val = 0b0110
    li t3, 0b0101
    bne t2, t3, fail
    lw t4, 0(t0)
    li t5, 0b0110
    bne t4, t5, fail

    # -------- amomin.w --------
    la t0, val_min
    li t1, -5
    amomin.w t2, t1, (t0)     # val = -5
    li t3, -3
    bne t2, t3, fail
    lw t4, 0(t0)
    li t5, -5
    bne t4, t5, fail

    # -------- amomax.w --------
    la t0, val_max
    li t1, 7
    amomax.w t2, t1, (t0)     # val = 7
    li t3, -3
    bne t2, t3, fail
    lw t4, 0(t0)
    li t5, 7
    bne t4, t5, fail

    # -------- amominu.w --------
    la t0, val_minu
    li t1, 7
    amominu.w t2, t1, (t0)    # val = 7
    li t3, -1   # 0xffffffff
    bne t2, t3, fail
    lw t4, 0(t0)
    li t5, 7
    bne t4, t5, fail

    # -------- amomaxu.w --------
    la t0, val_maxu
    li t1, 0xfffffffe
    amomaxu.w t2, t1, (t0)    # val = 0xfffffffe
    li t3, 1
    bne t2, t3, fail
    lw t4, 0(t0)
    li t5, 0xfffffffe
    bne t4, t5, fail

    # -------- amoadd.d (64-bit) --------
    la t0, val_add64
    li t1, 50
    amoadd.d t2, t1, (t0)     # val = 150
    li t3, 100
    bne t2, t3, fail
    ld t4, 0(t0)
    li t5, 150
    bne t4, t5, fail

    # -------- lr.w / sc.w --------
    la t0, val_lrsc32          # t0 = &mem
    lr.w t1, (t0)              # t1 = *mem (123)
    addi t2, zero, 456         # t2 = 456
    sc.w t3, t2, (t0)          # t3 = sc(*mem = 456)
    addi t4, zero, 0
    bne t3, t4, fail           # sc should return 0 (success)
    addi t5, zero, 123
    bne t1, t5, fail           # lr should have read 123
    lw t6, 0(t0)
    addi t5, zero, 456         # reuse t5 as comparison
    bne t6, t5, fail           # final value should be 456

    # -------- lr.d / sc.d --------
    la t0, val_lrsc64
    lr.d t1, (t0)              # t1 = 1000
    lui t2, 0
    addi t2, t2, 2000          # t2 = 2000
    sc.d t3, t2, (t0)          # store 2000 if still reserved
    addi t4, zero, 0
    bne t3, t4, fail
    lui t5, 0
    addi t5, t5, 1000
    bne t1, t5, fail           # compare loaded value
    ld t6, 0(t0)
    lui t5, 0
    addi t5, t5, 2000
    bne t6, t5, fail

    li a0, 0
    call halt

fail:
    li a0, 1
    call halt
