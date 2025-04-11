#include "cpu/riscv/core.h"
#include "cpu/riscv/def.h"
#include "cpu/riscv/namespace.h"
#include "cpu/word.h"
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
    std::lock_guard<std::mutex> lock(this->csrMtx);
    
    word_t mip = this->csr.read_csr(CSRAddr::MIP);
    mip |= 1 << code;
    this->csr.write_csr(CSRAddr::MIP, mip);
}

void RVCore::clear_interrupt(InterruptCode code) {
    std::lock_guard<std::mutex> lock(this->csrMtx);
    
    word_t mip = this->csr.read_csr(CSRAddr::MIP);
    mip &= ~(1 << code);
    this->csr.write_csr(CSRAddr::MIP, mip);
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
    this->csr.write_csr(CSRAddr::MEPC, this->npc);
    this->csr.write_csr(CSRAddr::MCAUSE, code | CAUSE_INTERRUPT_MASK);
    this->csr.write_csr(CSRAddr::MTVAL, 0);

    word_t mstatus = this->csr.read_csr(CSRAddr::MSTATUS);
    mstatus = (mstatus & ~STATUS_MPP_MASK) | (this->privMode << STATUS_MPP_OFF);
    mstatus = (mstatus & ~STATUS_MPIE_MASK) | ((mstatus & STATUS_MIE_MASK) << (STATUS_MPIE_OFF - STATUS_MIE_OFF));
    mstatus = (mstatus & ~STATUS_MIE_MASK);
    this->csr.write_csr(CSRAddr::MSTATUS, mstatus);

    this->privMode = PrivMode::MACHINE;

    word_t mtvec = this->csr.read_csr(CSRAddr::MTVEC);
    word_t vecMode = mtvec & TVEC_MODE_MASK;
    if (vecMode == TVECMode::VECTORED) {
        this->npc = (mtvec & ~TVEC_MODE_MASK) + (code << 2);
    } else {
        this->npc = mtvec & ~TVEC_MODE_MASK;
    }
}

void RVCore::interrupt_s(InterruptCode code) {
    this->csr.write_csr(CSRAddr::SEPC, this->npc);
    this->csr.write_csr(CSRAddr::SCAUSE, code | CAUSE_INTERRUPT_MASK);
    this->csr.write_csr(CSRAddr::STVAL, 0);

    word_t mstatus = this->csr.read_csr(CSRAddr::MSTATUS);
    mstatus = (mstatus & ~STATUS_SPP_MASK) | (this->privMode << STATUS_SPP_OFF);
    mstatus = (mstatus & ~STATUS_SPIE_MASK) | ((mstatus & STATUS_SIE_MASK) << (STATUS_SPIE_OFF - STATUS_SIE_OFF));
    mstatus = (mstatus & ~STATUS_SIE_MASK);
    this->write_csr(CSRAddr::MSTATUS, mstatus);

    this->privMode = PrivMode::SUPERVISOR;

    word_t stvec = this->csr.read_csr(CSRAddr::STVEC);
    word_t vecMode = stvec & TVEC_MODE_MASK;
    if (vecMode == TVECMode::VECTORED) {
        this->npc = (stvec & ~TVEC_MODE_MASK) + (code << 2);
    } else {
        this->npc = stvec & ~TVEC_MODE_MASK;
    }
}

static constexpr InterruptCode INTER_BITS[] = {
    // INTERRUPT_EXTERNAL_M,
    // INTERRUPT_SOFTWARE_M,
    // INTERRUPT_TIMER_M,
    // INTERRUPT_EXTERNAL_S,
    // INTERRUPT_SOFTWARE_S,
    // INTERRUPT_TIMER_S,
    InterruptCode::EXTERNAL_M,
    InterruptCode::SOFTWARE_M,
    InterruptCode::TIMER_M,
    InterruptCode::EXTERNAL_S,
    InterruptCode::SOFTWARE_S,
    InterruptCode::TIMER_S,
};

bool RVCore::scan_interrupt() {
    if (likely(this->privMode == PrivMode::MACHINE    && !(this->mstatus.mie))) return false;
    if (likely(this->privMode == PrivMode::SUPERVISOR && !(this->mstatus.sie))) return false;

    if (*this->mip == 0) return false;

    // Machine level interrupt
    word_t pending;
    if (this->mstatus.mie) {
        pending = *this->mip & *this->mie & ~*this->mideleg;
        if (pending) {
            for (unsigned int i = 0; i < sizeof(INTER_BITS) / sizeof(INTER_BITS[0]); i++) {
                if (pending & (1 << INTER_BITS[i])) {
                    interrupt_m(INTER_BITS[i]);
                    return true;
                }
            }
        }
    }
    
    // Supervisor level interrupt
    if (this->mstatus.sie) {
        pending = *this->mip & *this->mie & *this->mideleg;
        if (pending) {
            for (unsigned int i = 0; i < sizeof(INTER_BITS) / sizeof(INTER_BITS[0]); i++) {
                if (pending & (1 << INTER_BITS[i])) {
                    interrupt_s(INTER_BITS[i]);
                    return true;
                }
            }
        }
    }

    return false;
}
