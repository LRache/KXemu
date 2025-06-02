#include "cpu/riscv/core.h"
#include "cpu/riscv/csr-field.h"
#include "cpu/riscv/def.h"
#include "log.h"

using namespace kxemu::cpu;

void RVCore::set_priv_mode(int mode) {
    this->privMode = mode;
    this->update_vm_translate();
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
// implemented, else M). If y!=M, xRET also sets MPRV=0.
void RVCore::do_mret(const DecodeInfo &) {
    csr::MStatus mstatus = this->csr.get_csr_value(CSRAddr::MSTATUS);
    
    // change to previous privilege mode
    this->set_priv_mode(mstatus.mpp());
    
    // set mstatus.MIE to mstatus.MPIE
    mstatus.set_mie(mstatus.mpie());
    
    // set mstatus.MPIE to 1
    mstatus.set_mpie(true);

    // set mstatus.MPP to the lowest privilege mode
    mstatus.set_mpp(PrivMode::USER);

    this->csr.set_csr_value(CSRAddr::MSTATUS, mstatus);
    this->npc = this->csr.get_csr_value(CSRAddr::MEPC);
}

void RVCore::do_sret(const DecodeInfo &) {    
    csr::MStatus mstatus = this->csr.get_csr_value(CSRAddr::MSTATUS);

    // change to previous privilege mode
    this->set_priv_mode(mstatus.spp() ? PrivMode::SUPERVISOR : PrivMode::USER);

    // set mstatus.SIE to mstatus.SPIE
    mstatus.set_sie(mstatus.spie());

    // set mstatus.SPIE to 1
    mstatus.set_spie(true);

    // set mstatus.SPP to the lowest privilege mode
    mstatus.set_spp(PrivMode::USER);

    // set mstatus.MPRV to 0
    mstatus.set_mprv(0);

    this->csr.set_csr_value(CSRAddr::MSTATUS, mstatus);
    this->npc = this->csr.get_csr_value(CSRAddr::SEPC);
}

void RVCore::do_invalid_inst() {
    if (this->debugMode) {
        this->state = ERROR;
        this->haltPC = this->pc;
    }

    WARN("Invalid instruction at pc=" FMT_WORD ", inst=" FMT_WORD32, this->pc, this->inst);

    this->trap(TrapCode::ILLEGAL_INST, this->inst);
}

void RVCore::do_invalid_inst(const DecodeInfo &) {
    WARN("Invalid instruction at pc=" FMT_WORD ", inst=" FMT_WORD32, this->pc, this->inst);
    this->do_invalid_inst();
}

void RVCore::do_ebreak(const DecodeInfo &) {
    if (this->debugMode) {
        INFO("EBREAK at pc=" FMT_WORD, this->pc);
        this->state = HALT;
        this->haltCode = this->gpr[10];
        this->haltPC = this->pc;
    }
    
    this->trap(TrapCode::BREAKPOINT); 
}

void RVCore::do_wfi(const DecodeInfo &) {
    // INFO("WFI at pc=" FMT_WORD, this->pc);
    while (!this->scan_interrupt()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        this->update_device();
    }
}
