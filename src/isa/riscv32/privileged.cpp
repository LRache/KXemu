#include "debug.h"
#include "isa/riscv32/core.h"
#include "isa/riscv32/privileged-def.h"
#include "isa/riscv32/csr-def.h"
#include "isa/word.h"
#include "log.h"
#include "macro.h"


// To support nested traps, each privilege mode x that can respond to interrupts has a two-level stack of
// interrupt-enable bits and privilege modes. xPIE holds the value of the interrupt-enable bit active prior
// to the trap, and xPP holds the previous privilege mode. The xPP fields can only hold privilege modes
// up to x, so MPP is two bits wide and SPP is one bit wide. When a trap is taken from privilege mode y
// into privilege mode x, xPIE is set to the value of xIE; xIE is set to 0; and xPP is set to y.
void RV32Core::trap(word_t code, word_t value) {
    bool deleg;
    if (code >= 32) {
        deleg = *this->medelegh & (1 << (code - 32));
    } else {
        deleg = *this->medeleg & (1 << code);
    }

    word_t cause = code & ~CAUSE_INTERRUPT;
    word_t vec;
    if (deleg) {
        *this->sepc = this->pc;
        *this->scause = cause;
        *this->stval = value;
        vec = *this->stvec;

        word_t mstatus = *this->mstatus;
        mstatus = (mstatus & ~MSTATUS_SPP_MASK) | (this->privMode << MSTATUS_SPP_OFF);
        mstatus = (mstatus & ~MSTATUS_SPIE_MASK) | ((mstatus & MSTATUS_SIE_MASK) << (MSTATUS_SPIE_OFF - MSTATUS_SIE_OFF));
        mstatus = (mstatus & ~MSTATUS_SIE_MASK);
        *this->mstatus = mstatus;
    } else {
        *this->mepc = this->pc;
        *this->mcause = cause;
        *this->mtval = value;
        vec = *this->mtvec;

        word_t mstatus = *this->mstatus;
        mstatus = (mstatus & ~MSTATUS_MPP_MASK) | (this->privMode << MSTATUS_MPP_OFF);
        mstatus = (mstatus & ~MSTATUS_MPIE_MASK) | ((mstatus & MSTATUS_MIE_MASK) << (MSTATUS_MPIE_OFF - MSTATUS_MIE_OFF));
        mstatus = (mstatus & ~MSTATUS_MIE_MASK);
        *this->mstatus = mstatus;
    }

    word_t vecMode = vec & TVEC_MODE_MASK;
    if (vecMode == TVEC_MODE_VECTORED) {
        this->npc = (vec & ~TVEC_MODE_MASK) + (code << 2);
    } else {
        this->npc = vec & ~TVEC_MODE_MASK;
    }
}

void RV32Core::interrupt(word_t code) {
    *this->mip |= 1 << code;
}

void RV32Core::interrupt_m(word_t code) {
    *this->mip &= ~(1 << code);
    
    *this->mepc = this->pc;
    *this->mcause = code | CAUSE_INTERRUPT;
    *this->mtval = 0;

    word_t mstatus = *this->mstatus;
    mstatus = (mstatus & ~MSTATUS_MPP_MASK) | (this->privMode << MSTATUS_MPP_OFF);
    mstatus = (mstatus & ~MSTATUS_MPIE_MASK) | ((mstatus & MSTATUS_MIE_MASK) << (MSTATUS_MPIE_OFF - MSTATUS_MIE_OFF));
    mstatus = (mstatus & ~MSTATUS_MIE_MASK);
    *this->mstatus = mstatus;

    this->privMode = PrivMode::MACHINE;

    word_t vecMode = *this->mtvec & TVEC_MODE_MASK;
    if (vecMode == TVEC_MODE_VECTORED) {
        this->npc = (*this->mtvec & ~TVEC_MODE_MASK) + (code << 2);
    } else {
        this->npc = *this->mtvec & ~TVEC_MODE_MASK;
    }
}

void RV32Core::interrupt_s(word_t code) {
    *this->mip &= ~(1 << code);
    
    *this->sepc = this->pc;
    *this->scause = code | CAUSE_INTERRUPT;
    *this->stval = 0;

    word_t mstatus = *this->mstatus;
    mstatus = (mstatus & ~MSTATUS_SPP_MASK) | (this->privMode << MSTATUS_SPP_OFF);
    mstatus = (mstatus & ~MSTATUS_SPIE_MASK) | ((mstatus & MSTATUS_SIE_MASK) << (MSTATUS_SPIE_OFF - MSTATUS_SIE_OFF));
    mstatus = (mstatus & ~MSTATUS_SIE_MASK);
    *this->mstatus = mstatus;

    this->privMode = PrivMode::SUPERVISOR;

    word_t vecMode = *this->stvec & TVEC_MODE_MASK;
    if (vecMode == TVEC_MODE_VECTORED) {
        this->npc = (*this->stvec & ~TVEC_MODE_MASK) + (code << 2);
    } else {
        this->npc = *this->stvec & ~TVEC_MODE_MASK;
    }
}

bool RV32Core::scan_interrupt() {
    if (likely(this->privMode == PrivMode::MACHINE    && !(*this->mstatus & MSTATUS_MIE_MASK))) return false;
    if (likely(this->privMode == PrivMode::SUPERVISOR && !(*this->mstatus & MSTATUS_SIE_MASK))) return false;
    
    int si = -1;
    for (int i = 0; i < 32; i++) {
        word_t mask = 1 << i;
        if (unlikely(*this->mip & mask)) {
            if (*this->mideleg & mask) {
                if (unlikely(*this->sie & mask)) {
                    si = i;
                }
            } else if (*this->mie & mask) {
                this->interrupt_m(i);
                return true;
            }
        }
    }
    if (si != -1) {
        this->interrupt_s(si);
        return true;
    } else {
        return false;
    }
}

void RV32Core::do_ecall() {
    // ecall trap
    word_t code;
    switch (this->privMode) {
        case PrivMode::MACHINE:    code = 11; break;
        case PrivMode::SUPERVISOR: code = 9;  break;
        case PrivMode::USER:       code = 8;  break;
        default: code = 0; break;
    }
    trap(code); 
}

// An MRET or SRET instruction is used to return from a trap in M-mode or S-mode respectively. When
// executing an xRET instruction, supposing xPP holds the value y, xIE is set to xPIE; the privilege mode is
// changed to y; xPIE is set to 1; and xPP is set to the least-privileged supported mode (U if U-mode is
// implemented, else M). If y≠M, xRET also sets MPRV=0.
void RV32Core::do_mret() {
    word_t mstatus = *this->mstatus;
    
    // change to previous privilege mode
    word_t mpp = ((mstatus) >> MSTATUS_MPP_OFF) & 0x3;
    this->privMode = mpp;
    
    // set mstatus.MIE to mstatus.MPIE
    word_t mpie = (mstatus >> MSTATUS_MPIE_OFF) & 0x1;
    mstatus = (mstatus & ~MSTATUS_MIE_MASK) | (mpie << MSTATUS_MIE_OFF);
    
    // set mstatus.MPIE to 1
    mstatus |= MSTATUS_MPIE_MASK;

    // set mstatus.MPP to the lowest privilege mode
    mstatus = (mstatus & ~MSTATUS_MPP_MASK) | (PrivMode::USER << MSTATUS_MPP_OFF);

    *this->mstatus = mstatus;
    
    this->npc = *this->mepc;
    INFO("Switch to mode %d", this->privMode);
}

void RV32Core::do_sret() {    
    word_t mstatus = *this->mstatus;

    // change to previous privilege mode
    word_t spp = (mstatus >> MSTATUS_SPP_OFF) & 0x1;
    this->privMode = spp ? PrivMode::SUPERVISOR : PrivMode::USER;

    // set mstatus.SIE to mstatus.SPIE
    word_t spie = (mstatus >> MSTATUS_SPIE_OFF) & 0x1;
    mstatus = (mstatus & ~MSTATUS_SIE_MASK) | (spie << MSTATUS_SIE_OFF);

    // set mstatus.SPIE to 1
    mstatus |= MSTATUS_SPIE_MASK;

    // set mstatus.SPP to the lowest privilege mode
    mstatus &= ~(1 << MSTATUS_SPP_OFF);

    // set mstatus.MPRV to 0
    mstatus &= ~MSTATUS_MPRV_OFF;

    *this->mstatus = mstatus;
    this->npc = *this->sepc;
    INFO("Switch to mode %d", this->privMode);
}

void RV32Core::do_ebreak() {
    INFO("EBREAK at pc=" FMT_WORD, this->pc);
    this->state = BREAK;
    this->haltCode = this->gpr[10];
    this->haltPC = this->pc;
    
    // breakpoint trap
    this->trap(3); 
}
