#include "cpu/riscv/core.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"
#include "isa/word.h"
#include "log.h"

#include <ctime>

using namespace kxemu::cpu;

void RVCore::do_ecall() {
    word_t code;
    switch (this->privMode) {
        case PrivMode::MACHINE:    code = TRAP_ECALL_M; break;
        case PrivMode::SUPERVISOR: code = TRAP_ECALL_S; break;
        case PrivMode::USER:       code = TRAP_ECALL_U; break;
        default: PANIC("Invalid current privileged mode."); break;
    }
    trap(code); 
}

// An MRET or SRET instruction is used to return from a trap in M-mode or S-mode respectively. When
// executing an xRET instruction, supposing xPP holds the value y, xIE is set to xPIE; the privilege mode is
// changed to y; xPIE is set to 1; and xPP is set to the least-privileged supported mode (U if U-mode is
// implemented, else M). If y≠M, xRET also sets MPRV=0.
void RVCore::do_mret() {
    word_t mstatus = *this->mstatus;
    
    // change to previous privilege mode
    word_t mpp = ((mstatus) >> STATUS_MPP_OFF) & 0x3;
    this->privMode = mpp;
    
    // set mstatus.MIE to mstatus.MPIE
    word_t mpie = (mstatus >> STATUS_MPIE_OFF) & 0x1;
    mstatus = (mstatus & ~STATUS_MIE_MASK) | (mpie << STATUS_MIE_OFF);
    
    // set mstatus.MPIE to 1
    mstatus |= STATUS_MPIE_MASK;

    // set mstatus.MPP to the lowest privilege mode
    mstatus = (mstatus & ~STATUS_MPP_MASK) | (PrivMode::USER << STATUS_MPP_OFF);

    *this->mstatus = mstatus;
    
    this->npc = this->csr.read_csr(CSR_MEPC);
}

void RVCore::do_sret() {    
    word_t mstatus = *this->mstatus;

    // change to previous privilege mode
    word_t spp = (mstatus >> STATUS_SPP_OFF) & 0x1;
    this->privMode = spp ? PrivMode::SUPERVISOR : PrivMode::USER;

    // set mstatus.SIE to mstatus.SPIE
    word_t spie = (mstatus >> STATUS_SPIE_OFF) & 0x1;
    mstatus = (mstatus & ~STATUS_SIE_MASK) | (spie << STATUS_SIE_OFF);

    // set mstatus.SPIE to 1
    mstatus |= STATUS_SPIE_MASK;

    // set mstatus.SPP to the lowest privilege mode
    mstatus &= ~(1 << STATUS_SPP_OFF);

    // set mstatus.MPRV to 0
    mstatus &= ~STATUS_MPRV_OFF;

    *this->mstatus = mstatus;
    this->npc = this->csr.read_csr(CSR_SEPC);
}

void RVCore::do_ebreak() {
    INFO("EBREAK at pc=" FMT_WORD, this->pc);
    this->state = HALT;
    this->haltCode = this->gpr[10];
    this->haltPC = this->pc;
    
    // breakpoint trap
    this->trap(TRAP_BREAKPOINT); 
}

void RVCore::do_wfi() {
    while (!this->scan_interrupt()) {
        // wait for interrupt
    }
}

void RVCore::do_sfence_vma() {
    if (this->privMode == PrivMode::USER) {
        do_invalid_inst();
        return;
    } 
}
