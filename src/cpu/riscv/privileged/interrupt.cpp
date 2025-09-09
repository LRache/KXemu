#include "cpu/riscv/core.hpp"
#include "cpu/riscv/csr-field.hpp"
#include "cpu/riscv/def.hpp"
#include "cpu/word.hpp"
#include "macro.h"

using namespace kxemu::cpu;

void RVCore::update_stimecmp() {
    uint64_t stimecmp = this->csr.read_csr(CSRAddr::STIMECMP);
#ifdef KXEMU_ISA32
    stimecmp &= 0xffffffff;
    stimecmp |= (uint64_t)this->csr.read_csr(CSRAddr::STIMECMPH) << 32;
#endif
    this->aclint->register_stimer(this->coreID, stimecmp);
    this->clear_timer_interrupt_s();
}

void RVCore::set_interrupt(InterruptCode code) {
    csr::MIP mip = this->csr.get_csr_value(CSRAddr::MIP);
    mip.set_pending(code);
    this->csr.set_csr_value(CSRAddr::MIP, mip);
}

void RVCore::clear_interrupt(InterruptCode code) {
    csr::MIP mip = this->csr.get_csr_value(CSRAddr::MIP);
    mip.clear_pending(code);
    this->csr.set_csr_value(CSRAddr::MIP, mip);
}

void RVCore::set_timer_interrupt_m() {
    set_interrupt(InterruptCode::TIMER_M);
}

void RVCore::set_timer_interrupt_s() {
    set_interrupt(InterruptCode::TIMER_S);
}

void RVCore::clear_timer_interrupt_m() {
    clear_interrupt(InterruptCode::TIMER_M);
}

void RVCore::clear_timer_interrupt_s() {
    clear_interrupt(InterruptCode::TIMER_S);
}

void RVCore::set_software_interrupt_m() {
    set_interrupt(InterruptCode::SOFTWARE_M);
}

void RVCore::set_software_interrupt_s() {
    set_interrupt(InterruptCode::SOFTWARE_S);
}

void RVCore::clear_software_interrupt_m() {
    clear_interrupt(InterruptCode::SOFTWARE_M);
}

void RVCore::clear_software_interrupt_s() {
    clear_interrupt(InterruptCode::SOFTWARE_S);
}

void RVCore::set_external_interrupt_m() {
    set_interrupt(InterruptCode::EXTERNAL_M);
}

void RVCore::set_external_interrupt_s() {
    set_interrupt(InterruptCode::EXTERNAL_S);
}

void RVCore::clear_external_interrupt_m() {
    clear_interrupt(InterruptCode::EXTERNAL_M);
}

void RVCore::clear_external_interrupt_s() {
    clear_interrupt(InterruptCode::EXTERNAL_S);
}

void RVCore::interrupt_m(InterruptCode code) {
    this->csr.set_csr_value(CSRAddr::MEPC, this->npc);
    this->csr.set_csr_value(CSRAddr::MTVAL, 0);
    this->csr.set_csr_value(CSRAddr::MCAUSE, csr::MCause(code));

    csr::MStatus mstatus = this->csr.get_csr_value(CSRAddr::MSTATUS);
    mstatus.set_mpp(this->privMode);
    mstatus.set_mpie(mstatus.mie());
    mstatus.set_mie(false);
    this->csr.set_csr_value(CSRAddr::MSTATUS, mstatus);

    this->set_priv_mode(PrivMode::MACHINE);

    csr::TrapVec mtvec = this->csr.get_csr_value(CSRAddr::MTVEC);
    if (mtvec.mode() == csr::TrapVec::VECTORED) {
        this->npc = (mtvec.vec()) + (code << 2);
    } else {
        this->npc = mtvec.vec();
    }
}

void RVCore::interrupt_s(InterruptCode code) {
    this->csr.write_csr(CSRAddr::SEPC, this->npc);
    this->csr.write_csr(CSRAddr::SCAUSE, csr::MCause(code));
    this->csr.write_csr(CSRAddr::STVAL, 0);

    csr::MStatus mstatus = this->csr.read_csr(CSRAddr::MSTATUS);
    mstatus.set_spp(this->privMode != PrivMode::USER);
    mstatus.set_spie(mstatus.sie());
    mstatus.set_sie(false);
    this->write_csr(CSRAddr::MSTATUS, mstatus);

    this->set_priv_mode(PrivMode::SUPERVISOR);

    csr::TrapVec stvec = this->csr.get_csr_value(CSRAddr::STVEC);
    if (stvec.mode() == csr::TrapVec::VECTORED) {
        this->npc = stvec.vec() + (code << 2);
    } else {
        this->npc = stvec.vec();
    }
}

// Multiple simultaneous interrupts destined for M-mode are handled in the following decreasing priority
// order: MEI, MSI, MTI, SEI, SSI, STI, LCOFI.
static constexpr InterruptCode INTER_BITS_M[] = {
    InterruptCode::EXTERNAL_M,    // MEI
    InterruptCode::SOFTWARE_M,    // MSI
    InterruptCode::TIMER_M,       // MTI
    InterruptCode::EXTERNAL_S,    // SEI
    InterruptCode::SOFTWARE_S,    // SSI
    InterruptCode::TIMER_S,       // STI
};

static constexpr InterruptCode INTER_BITS_S[] = {
    InterruptCode::EXTERNAL_S,    // SEI
    InterruptCode::SOFTWARE_S,    // SSI
    InterruptCode::TIMER_S,       // STI
};

bool RVCore::scan_interrupt() {
    // There is no interrput pending.
    if (likely(*this->mip == 0)) return false;

    // Machine Level Interrupt
    // An interrupt `i` will trap to M-mode (causing the privilege mode to change to M-mode) if all of the
    // following are true: 
    // (a) either the current privilege mode is M and the MIE bit in the mstatus register is
    //     set, or the current privilege mode has less privilege than M-mode; 
    // (b) bit `i` is set in both mip and mie;
    // (c) if register mideleg exists, bit `i` is not set in mideleg
    // -- from The RISC-V Instruction Set Manual: Volume II 3.1.9  Page 42
    if (this->privMode < PrivMode::MACHINE || this->mstatus.mie) {
        word_t pending = *this->mip & *this->mie & ~*this->mideleg;
        if (pending) {
            for (auto const &bit : INTER_BITS_M) {
                if (pending & (1 << bit)) {
                    interrupt_m(bit);
                    return true;
                }
            }
        }
    }

    // Supervisor level interrupt
    // An interrupt `i` will trap to S-mode if all of the following are true: 
    // (a) either the current privilege mode is S and the SIE bit in the sstatus register is set, 
    //     or the current privilege mode has less privilege than S-mode; 
    // (b) bit `i` is set in both sip and sie; and
    // -- from The RISC-V Instruction Set Manual: Volume II 12.1.3  Page 119
    if (this->privMode < PrivMode::SUPERVISOR || (this->privMode ==  PrivMode::SUPERVISOR && this->mstatus.sie)) {
        word_t pending = *this->mip & *this->mie & *this->mideleg;
        if (pending) {
            for (auto const &bit : INTER_BITS_S) {
                if (pending & (1 << bit)) {
                    interrupt_s(bit);
                    return true;
                }
            }
        }
    }

    return false;
}
