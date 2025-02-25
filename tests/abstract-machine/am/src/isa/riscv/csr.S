.section .text
.globl csrr_mhartid
.globl csrr_time
.globl csrr_mstatus
.globl csrr_mie
.globl csrr_mcause
.globl csrr_mepc
.globl csrr_medeleg

.globl csrw_pmpcfg0
.globl csrw_pmpaddr0
.globl csrw_mstatus
.globl csrw_mcause
.globl csrw_mie
.globl csrw_mepc
.globl csrw_mtvec
.globl csrw_medeleg
.globl csrw_mscratch
.globl csrw_sepc
.globl csrw_stvec
.globl csrw_satp

.globl mret
.globl sret
.globl ecall
.globl wfi
.globl sfence_vma

csrr_mhartid:
    csrr a0, mhartid
    ret

csrr_time:
    csrr a0, time
    ret

csrr_mstatus:
    csrr a0, mstatus
    ret

csrr_mie:
    csrr a0, mie
    ret

csrr_mcause:
    csrr a0, mcause
    ret

csrr_mepc:
    csrr a0, mepc
    ret

csrr_medeleg:
    csrr a0, medeleg
    ret

csrw_pmpcfg0:
    csrw pmpcfg0, a0
    ret

csrw_pmpaddr0:
    csrw pmpaddr0, a0
    ret

csrw_mstatus:
    csrw mstatus, a0
    ret

csrw_mie:
    csrw mie, a0
    ret

csrw_mcause:
    csrw mcause, a0
    ret

csrw_mepc:
    csrw mepc, a0
    ret

csrw_mtvec:
    csrw mtvec, a0
    ret

csrw_medeleg:
    csrw medeleg, a0
    ret

csrw_mscratch:
    csrw mscratch, a0
    ret

csrw_sepc:
    csrw sepc, a0
    ret

csrw_stvec:
    csrw stvec, a0
    ret

csrw_satp:
    csrw satp, a0
    ret

mret:
    mret

sret:
    sret

ecall:
    ecall
    ret

wfi:
    wfi
    ret

sfence_vma:
    sfence.vma
    ret