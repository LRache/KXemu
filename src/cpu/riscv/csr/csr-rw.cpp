#include "cpu/riscv/csr.h"
#include "cpu/riscv/csr-field.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"
#include "debug.h"
#include "log.h"

#include <optional>

using namespace kxemu::cpu;

word_t RVCSR::get_csr_value(CSRAddr addr) const {
    auto iter = this->csr.find(addr);
    SELF_PROTECT(iter != this->csr.end(), "Access to non-exist CSR 0x%03x", addr);
    return iter->second.value;
}

void RVCSR::set_csr_value(CSRAddr addr, word_t value) {
    auto iter = this->csr.find(addr);
    SELF_PROTECT(iter != this->csr.end(), "Access to non-exist CSR 0x%03x", addr);
    iter->second.value = value;
}

#define INTER_MASK_M \
        ((1 << InterruptCode::SOFTWARE_S) | \
         (1 << InterruptCode::SOFTWARE_M) | \
         (1 << InterruptCode::TIMER_S)    | \
         (1 << InterruptCode::TIMER_M)    | \
         (1 << InterruptCode::EXTERNAL_S) | \
         (1 << InterruptCode::EXTERNAL_M))

#define INTER_MASK_S \
        ((1 << InterruptCode::SOFTWARE_S) | \
         (1 << InterruptCode::TIMER_S)    | \
         (1 << InterruptCode::EXTERNAL_S))

// Bits in MIP register
// NAME   | MIP
// MEIP   | ReadOnly
// MTIP   | ReadOnly
// MSIP   | ReadOnly
// STIP   | ReadWrite
// SSIP   | ReadWrite
// SEIP   | ReadOnly
// LCOFIP | ReadOnly
bool RVCSR::write_mip(unsigned int addr, word_t value) {
    csr::MIP mip = this->get_csr_value(CSRAddr::MIP);
    mip.set_mip(value);
    this->set_csr_value(CSRAddr::MIP, mip); 
    return true;
}

bool RVCSR::write_mie(unsigned int addr, word_t value) {
    // this->set_csr_value(CSRAddr::MIE, value & INTER_MASK_M);
    this->set_csr_value(CSRAddr::MIE, csr::MIE(value));
    return true;
}

std::optional<word_t> RVCSR::read_sip(unsigned int addr) {
    csr::MIP mip = this->get_csr_value(CSRAddr::MIP);
    return mip.sip();
}

bool RVCSR::write_sip(unsigned int addr, word_t value) {
    csr::MIP mip = this->get_csr_value(CSRAddr::MIP);
    mip.set_sip(value);
    this->set_csr_value(CSRAddr::MIP, mip);
    return true;
}

std::optional<word_t> RVCSR::read_sie(unsigned int addr) {
    csr::MIE mie = this->get_csr_value(CSRAddr::MIE);
    return mie.sie();
}

bool RVCSR::write_sie(unsigned int addr, word_t value) {
    csr::MIE mie = this->get_csr_value(CSRAddr::MIE);
    mie.set_sie(value);
    this->set_csr_value(CSRAddr::MIE, mie);
    return true;
}

std::optional<word_t> RVCSR::read_sstatus(unsigned int addr) {
    csr::MStatus mstatus = this->get_csr_value(CSRAddr::MSTATUS);
    return mstatus.sstatus();
}

bool RVCSR::write_sstatus(unsigned int addr, word_t value) {
    csr::MStatus mstatus = this->get_csr_value(CSRAddr::MSTATUS);
    mstatus.set_sstatus(value);
    this->set_csr_value(CSRAddr::MSTATUS, mstatus);
    return true;
}

bool RVCSR::write_pmpcfg(unsigned int addr, word_t value) {
    this->set_csr_value((CSRAddr)addr, value);
    this->reload_pmpcfg();
    return true;
}

bool RVCSR::write_pmpaddr(unsigned int addr, word_t value) {
    this->set_csr_value((CSRAddr)addr, csr::PMPAddr(value));
    this->reload_pmpcfg();
    return true;
}

bool RVCSR::write_satp(unsigned int, word_t value) {
    csr::Satp satp = value;
    
    // ASID is WARL, so we need to set it to 0
    satp.set_asid(0);
    
    this->set_csr_value(CSRAddr::SATP, satp);

    return true;
}

std::optional<word_t> RVCSR::read_fflags(unsigned int addr) {
    csr::FCSR fcsr = this->get_csr_value(CSRAddr::FCSR);
    return fcsr.fflags();
}

bool RVCSR::write_fflags(unsigned int addr, word_t value) {
    csr::FCSR fcsr = this->get_csr_value(CSRAddr::FCSR);
    fcsr.set_fflags(value);
    this->set_csr_value(CSRAddr::FCSR, fcsr);
    return true;
}

std::optional<word_t> RVCSR::read_frm(unsigned int addr) {
    csr::FCSR fcsr = this->get_csr_value(CSRAddr::FCSR);
    return fcsr.frm();
}

bool RVCSR::write_frm(unsigned int addr, word_t value) {
    csr::FCSR fcsr = this->get_csr_value(CSRAddr::FCSR);
    fcsr.set_frm(value);
    this->set_csr_value(CSRAddr::FCSR, fcsr);
    return true;
}

bool RVCSR::time_readable() {
//     csr::MCounteren mcnten = this->get_csr_value(CSRAddr::MCNTEN);

//     // Check if the timer is enabled
//     if (!mcnten.tm() && this->privMode != PrivMode::MACHINE) {
//         return false;
//     }

//     csr::MCounteren scnten = this->get_csr_value(CSRAddr::SCNTEN);
//     if (!scnten.tm() && this->privMode == PrivMode::USER) {
//         return false;
//     }

//     bool sstc;
// #ifdef KXEMU_ISA32
//     csr::MEnvConfigH menvcfgh = this->csr[CSRAddr::MENVCFGH].value;
//     sstc = menvcfgh.stce();
// #else
//     csr::MEnvConfig menvcfg = this->csr[CSRAddr::MENVCFG].value;
//     sstc = menvcfg.stce();
// #endif
    
//     if (!sstc && this->privMode != PrivMode::MACHINE) {
//         return false;
//     }

    return true;
}

std::optional<word_t> RVCSR::read_time(unsigned int addr) {
    if (!this->time_readable()) {
        return std::nullopt;
    }

    uint64_t mtime = realtime_to_mtime(this->get_uptime());
    return mtime;
}

std::optional<word_t> RVCSR::read_timeh(unsigned int addr) {
    if (!this->time_readable()) {
        return std::nullopt;
    }

    uint64_t mtime = realtime_to_mtime(this->get_uptime());
    return mtime >> 32;
}
