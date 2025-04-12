#include "cpu/riscv/core.h"
#include "cpu/riscv/csr-field.h"
#include "cpu/riscv/def.h"
#include "log.h"

using namespace kxemu::cpu;

void RVCore::set_priv_mode(int mode) {
    // Change the virtual address translation function when entering or exiting machine mode
    if (this->privMode != PrivMode::MACHINE && mode == PrivMode::MACHINE) {
        if (this->vaddr_translate_func != &RVCore::vaddr_translate_bare) {
            this->vaddr_translate_func = &RVCore::vaddr_translate_bare;
            this->icache_fence();
        }
    }

    this->privMode = mode;
}

void RVCore::do_ecall(const DecodeInfo &) {
    TrapCode code;
    switch (this->privMode) {
        case PrivMode::MACHINE:    code = TrapCode::ECALL_M; break;
        case PrivMode::SUPERVISOR: code = TrapCode::ECALL_S; break;
        case PrivMode::USER:       code = TrapCode::ECALL_U; break;
        default: PANIC("Invalid current privileged mode."); return;
    }
    trap(code); 
}

// An MRET or SRET instruction is used to return from a trap in M-mode or S-mode respectively. When
// executing an xRET instruction, supposing xPP holds the value y, xIE is set to xPIE; the privilege mode is
// changed to y; xPIE is set to 1; and xPP is set to the least-privileged supported mode (U if U-mode is
// implemented, else M). If y≠M, xRET also sets MPRV=0.
void RVCore::do_mret(const DecodeInfo &) {
    csr::MStatus mstatus = this->get_csr_core(CSRAddr::MSTATUS);
    
    // change to previous privilege mode
    // word_t mpp = ((mstatus) >> STATUS_MPP_OFF) & 0x3;
    this->set_priv_mode(mstatus.mpp());
    
    // set mstatus.MIE to mstatus.MPIE
    // word_t mpie = (mstatus >> STATUS_MPIE_OFF) & 0x1;
    // mstatus = (mstatus & ~STATUS_MIE_MASK) | (mpie << STATUS_MIE_OFF);
    mstatus.set_mie(mstatus.mpie());
    
    // set mstatus.MPIE to 1
    // mstatus |= STATUS_MPIE_MASK;
    mstatus.set_mpie(true);

    // set mstatus.MPP to the lowest privilege mode
    // mstatus = (mstatus & ~STATUS_MPP_MASK) | (PrivMode::USER << STATUS_MPP_OFF);
    mstatus.set_mpp(PrivMode::USER);

    this->set_csr_core(CSRAddr::MSTATUS, mstatus);
    this->npc = this->get_csr_core(CSRAddr::MEPC);
}

void RVCore::do_sret(const DecodeInfo &) {    
    csr::MStatus mstatus = this->get_csr_core(CSRAddr::MSTATUS);

    // change to previous privilege mode
    // word_t spp = (mstatus >> STATUS_SPP_OFF) & 0x1;
    // this->set_priv_mode(spp ? PrivMode::SUPERVISOR : PrivMode::USER);
    this->set_priv_mode(mstatus.spp() ? PrivMode::SUPERVISOR : PrivMode::USER);

    // set mstatus.SIE to mstatus.SPIE
    // word_t spie = (mstatus >> STATUS_SPIE_OFF) & 0x1;
    // mstatus = (mstatus & ~STATUS_SIE_MASK) | (spie << STATUS_SIE_OFF);
    mstatus.set_sie(mstatus.spie());

    // set mstatus.SPIE to 1
    // mstatus |= STATUS_SPIE_MASK;
    mstatus.set_spie(true);

    // set mstatus.SPP to the lowest privilege mode
    // mstatus &= ~(1 << STATUS_SPP_OFF);
    mstatus.set_spp(PrivMode::USER);

    // set mstatus.MPRV to 0
    // mstatus &= ~STATUS_MPRV_OFF;
    mstatus.set_mprv(0);

    this->set_csr_core(CSRAddr::MSTATUS, mstatus);
    this->npc = this->get_csr_core(CSRAddr::SEPC);
}

void RVCore::do_ebreak(const DecodeInfo &) {
    // INFO("EBREAK at pc=" FMT_WORD, this->pc);
    this->state = HALT;
    this->haltCode = this->gpr[10];
    this->haltPC = this->pc;
    
    // breakpoint trap
    this->trap(TrapCode::BREAKPOINT); 
}

void RVCore::do_wfi(const DecodeInfo &) {
    while (!this->scan_interrupt()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        this->update_device();
    }
}
