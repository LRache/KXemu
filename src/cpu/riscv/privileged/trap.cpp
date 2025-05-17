#include "cpu/riscv/core.h"
#include "cpu/riscv/csr-field.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"
#include "log.h"

using namespace kxemu::cpu;

// To support nested traps, each privilege mode x that can respond to interrupts has a two-level stack of
// interrupt-enable bits and privilege modes. xPIE holds the value of the interrupt-enable bit active prior
// to the trap, and xPP holds the previous privilege mode. The xPP fields can only hold privilege modes
// up to x, so MPP is two bits wide and SPP is one bit wide. When a trap is taken from privilege mode y
// into privilege mode x, xPIE is set to the value of xIE; xIE is set to 0; and xPP is set to y.
void RVCore::trap(TrapCode cause, word_t value) {
    bool deleg;
#ifdef KXEMU_ISA64
    deleg = *this->medeleg & (1 << cause);
#else
    if (code >= 32) {
        deleg = *this->medelegh & (1 << (code - 32));
    } else {
        deleg = *this->medeleg & (1 << code);
    }
#endif

    CSRAddr epcAddr, causeAddr, tvalAddr, vecAddr;
    csr::MStatus mstatus = this->csr.get_csr_value(CSRAddr::MSTATUS);
    if (deleg) {
        epcAddr   = CSRAddr::SEPC;
        causeAddr = CSRAddr::SCAUSE;
        tvalAddr  = CSRAddr::STVAL;
        vecAddr   = CSRAddr::STVEC;

        mstatus.set_spp(this->privMode != PrivMode::USER);
        mstatus.set_spie(mstatus.sie());  
        mstatus.set_sie(false);

        this->set_priv_mode(PrivMode::SUPERVISOR);
    } else {
        epcAddr   = CSRAddr::MEPC;
        causeAddr = CSRAddr::MCAUSE;
        tvalAddr  = CSRAddr::MTVAL;
        vecAddr   = CSRAddr::MTVEC;

        mstatus.set_mpp(this->privMode);
        mstatus.set_mpie(mstatus.mie());
        mstatus.set_mie(false);
        
        this->set_priv_mode(PrivMode::MACHINE);
    }

    this->csr.set_csr_value(epcAddr, this->pc);
    this->csr.set_csr_value(causeAddr, cause);
    this->csr.set_csr_value(tvalAddr, value);
    this->csr.set_csr_value(CSRAddr::MSTATUS, mstatus);

    DEBUG("Trap: " FMT_WORD ", EPC: " FMT_WORD ", TVAL: " FMT_WORD, cause, this->pc, value);

    csr::TrapVec vec = this->csr.get_csr_value(vecAddr);
    if (vec.mode() == csr::TrapVec::VECTORED) {
        this->npc = vec.vec() + (cause << 2);
    } else {
        this->npc = vec.vec();
    }
}
