#include "cpu/riscv/core.h"
#include "cpu/riscv/csr-field.h"
#include "cpu/word.h"

using namespace kxemu::cpu;

// To support nested traps, each privilege mode x that can respond to interrupts has a two-level stack of
// interrupt-enable bits and privilege modes. xPIE holds the value of the interrupt-enable bit active prior
// to the trap, and xPP holds the previous privilege mode. The xPP fields can only hold privilege modes
// up to x, so MPP is two bits wide and SPP is one bit wide. When a trap is taken from privilege mode y
// into privilege mode x, xPIE is set to the value of xIE; xIE is set to 0; and xPP is set to y.
void RVCore::trap(TrapCode code, word_t value) {
    bool deleg;
#ifdef KXEMU_ISA64
    deleg = *this->medeleg & (1 << code);
#else
    if (code >= 32) {
        deleg = *this->medelegh & (1 << (code - 32));
    } else {
        deleg = *this->medeleg & (1 << code);
    }
#endif

    // word_t cause = code & ~CAUSE_INTERRUPT_MASK;
    word_t cause = code;
    csr::TrapVec vec = 0;
    if (deleg) {
        this->set_csr_core(CSRAddr::SEPC, this->pc);
        this->set_csr_core(CSRAddr::SCAUSE, cause);
        this->set_csr_core(CSRAddr::STVAL, value);
        vec = this->get_csr_core(CSRAddr::STVEC);

        csr::MStatus mstatus = this->get_csr_core(CSRAddr::MSTATUS);
        
        // mstatus = (mstatus & ~STATUS_SPP_MASK) | ((this->privMode != PrivMode::USER) << STATUS_SPP_OFF);
        mstatus.set_spp(this->privMode != PrivMode::USER);
        
        // mstatus = (mstatus & ~STATUS_SPIE_MASK) | ((mstatus & STATUS_SIE_MASK) << (STATUS_SPIE_OFF - STATUS_SIE_OFF));
        mstatus.set_spie(mstatus.sie());
        
        // mstatus = (mstatus & ~STATUS_SIE_MASK);
        mstatus.set_sie(false);
        
        this->set_csr_core(CSRAddr::MSTATUS, mstatus);

        // this->privMode = PrivMode::SUPERVISOR;
        this->set_priv_mode(PrivMode::SUPERVISOR);
    } else {
        this->set_csr_core(CSRAddr::MEPC, this->pc);
        this->set_csr_core(CSRAddr::MCAUSE, cause);
        this->set_csr_core(CSRAddr::MTVAL, value);
        vec = this->csr.read_csr(CSRAddr::MTVEC);

        csr::MStatus mstatus = this->csr.read_csr(CSRAddr::MSTATUS);
        
        // mstatus = (mstatus & ~STATUS_MPP_MASK) | (this->privMode << STATUS_MPP_OFF);
        mstatus.set_mpp(this->privMode);
        
        // mstatus = (mstatus & ~STATUS_MPIE_MASK) | ((mstatus & STATUS_MIE_MASK) << (STATUS_MPIE_OFF - STATUS_MIE_OFF));
        mstatus.set_mpie(mstatus.mie());

        // mstatus = (mstatus & ~STATUS_MIE_MASK);
        mstatus.set_mie(false);
        
        this->set_csr_core(CSRAddr::MSTATUS, mstatus);

        // this->privMode = PrivMode::MACHINE;
        this->set_priv_mode(PrivMode::MACHINE);
    }

    // word_t vecMode = vec & TVEC_MODE_MASK;
    if (vec.mode() == TVECMode::VECTORED) {
        // this->npc = (vec & ~TVEC_MODE_MASK) + (code << 2);
        this->npc = vec.vec() + (code << 2);
    } else {
        // this->npc = vec & ~TVEC_MODE_MASK;
        this->npc = vec.vec();
    }
}
