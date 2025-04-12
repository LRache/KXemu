#include "cpu/riscv/csr.h"
#include "cpu/riscv/csr-field.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"

using namespace kxemu::cpu;

word_t RVCSR::read_misa(unsigned int addr, word_t value, bool &valid) {
    return value;
}

word_t RVCSR::write_misa(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    // Field MISA_E is read-only
    // if (value & MISAFlag::I) {
    //     value |= MISA_E;
    // }
    // // Field MISA_D depends on the value of MISA_F
    // if (!(value & MISA_F)) {
    //     value &= ~MISA_D;
    // }
    value = this->csr[CSRAddr::MISA].value;
    return value;
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
word_t RVCSR::read_mip(unsigned int addr, word_t value, bool &valid) {  
    return value & INTER_MASK_M;
}

word_t RVCSR::write_mip(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    return value & ((1 << InterruptCode::SOFTWARE_S) | (1 << InterruptCode::TIMER_S));
}

word_t RVCSR::read_mie(unsigned int addr, word_t value, bool &valid) {
    return value & INTER_MASK_M;
}

word_t RVCSR::write_mie(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    return value & INTER_MASK_M;
}

word_t RVCSR::read_sip(unsigned int addr, word_t value, bool &valid) {
    return this->csr[CSRAddr::MIE].value & INTER_MASK_S;
}

word_t RVCSR::write_sip(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    value &= (1 << InterruptCode::SOFTWARE_S);
    this->csr[CSRAddr::MIP].value &= ~(1 << InterruptCode::SOFTWARE_S);
    this->csr[CSRAddr::MIP].value |= value;
    return 0;
}

word_t RVCSR::read_sie(unsigned int addr, word_t value, bool &valid) {
    return this->csr[CSRAddr::MIE].value & INTER_MASK_S;
}

word_t RVCSR::write_sie(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    this->csr[CSRAddr::MIE].value &= ~(INTER_MASK_S);
    this->csr[CSRAddr::MIE].value |= value & INTER_MASK_S;
    return 0;
}

word_t RVCSR::read_sstatus(unsigned int addr, word_t value, bool &valid) {
    return csr::MStatus(this->csr[CSRAddr::MSTATUS].value).sstatus();
}

word_t RVCSR::write_sstatus(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    csr::MStatus mstatus = this->csr[CSRAddr::MSTATUS].value;
    mstatus.set_sstatus(value);
    this->csr[CSRAddr::MSTATUS].value = mstatus;
    return 0;
}

word_t RVCSR::write_stimecmp(unsigned int addr, word_t value, bool &valid) {
    this->csr[CSRAddr::STIMECMP].value = value;
    valid = true;
    return value;
}

word_t RVCSR::write_pmpcfg(unsigned int addr, word_t value, bool &valid) {
    csr[addr].value = value;
    reload_pmpcfg();
    valid = true;
    return value;
}

word_t RVCSR::write_pmpaddr(unsigned int addr, word_t value, bool &valid) {
    csr[addr].value = value;
    reload_pmpcfg();
    valid = true;
    return value;
}

word_t RVCSR::write_satp(unsigned int addr, word_t value, bool &valid) {
    // value &= ~SATP_ASID_MASK; // ASID is WARL
    
    csr::Satp satp = value;
    
    // ASID is WARL, so we need to set it to 0
    satp.set_asid(0);

    #ifdef KXEMU_ISA64
    // Check if the mode is valid
    static constexpr word_t validSatpMode64[] = {
        SATPMode::BARE,
        SATPMode::SV39,
        SATPMode::SV48,
        SATPMode::SV57
    };
    word_t mode = satp.mode();
    bool flag = true;
    for (unsigned int i = 0; i < sizeof(validSatpMode64) / sizeof(word_t); i++) {
        if (mode == validSatpMode64[i]) {
            flag = false;
            break;
        }
    }
    if (flag) {
        // value &= ~SATP_MODE_MASK; // Set mode to Bare
        satp.set_mode(SATPMode::BARE);
    }
    #endif
    
    valid = true;
    return satp;
}

word_t RVCSR::read_fflags(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    csr::FCSR fcsr = this->read_csr(CSRAddr::FCSR);
    return fcsr.fflags();
}

word_t RVCSR::write_fflags(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    csr::FCSR fcsr = this->read_csr(CSRAddr::FCSR);
    fcsr.set_fflags(value);
    // fcsr = (fcsr & ~FCSR_FLAGS_MASK) | (value & 0x1f);
    this->write_csr(CSRAddr::FCSR, fcsr);
    return 0;
}

word_t RVCSR::read_frm(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    csr::FCSR fcsr = this->read_csr(CSRAddr::FCSR);
    return fcsr.frm();
}

word_t RVCSR::write_frm(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    csr::FCSR fcsr = this->read_csr(CSRAddr::FCSR);
    // fcsr = (fcsr & ~FCSR_RM_MASK) | (value & 0x7);
    fcsr.set_frm(value);
    this->write_csr(CSRAddr::FCSR, fcsr);
    return 0;
}

word_t RVCSR::read_time(unsigned int addr, word_t value, bool &valid) {
    csr::MCounteren mcnten = this->csr[CSRAddr::MCNTEN].value;

    // Check if the timer is enabled
    if (!mcnten.tm() && this->privMode != PrivMode::MACHINE) {
        valid = false;
        return 0;
    }

    bool sstc;
#ifdef KXEMU_ISA32
    csr::MEnvConfigH menvcfgh = this->csr[CSRAddr::MENVCFGH].value;
    sstc = menvcfgh.stce();
#else
    csr::MEnvConfig menvcfg = this->csr[CSRAddr::MENVCFG].value;
    sstc = menvcfg.stce();
#endif
    if (!sstc && this->privMode != PrivMode::MACHINE) {
        valid = false;
        return 0;
    }

    uint64_t mtime = uptime_to_mtime(this->get_uptime());
#ifdef KXEMU_ISA32
    csr[CSRAddr::TIMEH].value = mtime >> 32;
#endif
    return mtime;
}
