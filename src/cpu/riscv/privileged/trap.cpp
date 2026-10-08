#include "cpu/riscv/core.hpp"
#include "cpu/riscv/csr-field.hpp"
#include "cpu/riscv/def.hpp"
#include "cpu/word.hpp"

using namespace kxemu::cpu;

// To support nested traps, each privilege mode x that can respond to interrupts has a two-level stack of
// interrupt-enable bits and privilege modes. xPIE holds the value of the interrupt-enable bit active prior
// to the trap, and xPP holds the previous privilege mode. The xPP fields can only hold privilege modes
// up to x, so MPP is two bits wide and SPP is one bit wide. When a trap is taken from privilege mode y
// into privilege mode x, xPIE is set to the value of xIE; xIE is set to 0; and xPP is set to y.
void RVCore::enter_trap(TrapCode cause, word_t value) {
    bool deleg;

    if (unlikely(this->privMode == PrivMode::MACHINE)) {
        deleg = false;
    } else {
#ifdef KXEMU_ISA64
        deleg = this->csr.medeleg & (1 << cause);
#else
        if (cause >= 32) {
            deleg = this->csr.medelegh & (1 << (cause - 32));
        } else {
            deleg = this->csr.medeleg & (1 << cause);
        }
#endif
    }

    csr::MStatus mstatus = this->csr.mstatus;
    if (deleg) {
        mstatus.set_spp(this->privMode != PrivMode::USER);
        mstatus.set_spie(mstatus.sie());  
        mstatus.set_sie(false);

        this->set_priv_mode(PrivMode::SUPERVISOR);
    } else {
        mstatus.set_mpp(this->privMode);
        mstatus.set_mpie(mstatus.mie());
        mstatus.set_mie(false);
        
        this->set_priv_mode(PrivMode::MACHINE);
    }

    // Trap CSRs of the target privilege mode.
    word_t &epc    = deleg ? this->csr.sepc   : this->csr.mepc;
    word_t &tcause = deleg ? this->csr.scause : this->csr.mcause;
    word_t &tval   = deleg ? this->csr.stval  : this->csr.mtval;
    word_t &tinst  = deleg ? this->csr.stinst : this->csr.mtinst;
    const csr::TrapVec vec = deleg ? this->csr.stvec : this->csr.mtvec;

    epc    = this->pc;
    tcause = cause;
    tval   = value;
    tinst  = this->inst;
    this->csr.mstatus = mstatus;

    if (vec.mode() == csr::TrapVec::VECTORED) {
        this->npc = vec.vec() + (cause << 2);
    } else {
        this->npc = vec.vec();
    }
}
