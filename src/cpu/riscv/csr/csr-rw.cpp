#include "cpu/riscv/csr.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"

using namespace kxemu::cpu;

word_t RVCSR::read_misa(unsigned int addr, word_t value) {
    return value;
}

word_t RVCSR::write_misa(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    // Field MISA_E is read-only
    if (value & MISA_I) {
        value |= MISA_E;
    }
    // Field MISA_D depends on the value of MISA_F
    if (!(value & MISA_F)) {
        value &= ~MISA_D;
    }
    return value;
}

#define INTER_MASK_M \
        ((1 << INTERRUPT_SOFTWARE_S) | \
         (1 << INTERRUPT_SOFTWARE_M) | \
         (1 << INTERRUPT_TIMER_S)    | \
         (1 << INTERRUPT_TIMER_M)    | \
         (1 << INTERRUPT_EXTERNAL_S) | \
         (1 << INTERRUPT_EXTERNAL_M))

#define INTER_MASK_S \
        ((1 << INTERRUPT_SOFTWARE_S) | \
         (1 << INTERRUPT_TIMER_S)    | \
         (1 << INTERRUPT_EXTERNAL_S))

// Bits in MIP register
// NAME   | MIP
// MEIP   | ReadOnly
// MTIP   | ReadOnly
// MSIP   | ReadOnly
// STIP   | ReadWrite
// SSIP   | ReadWrite
// SEIP   | ReadOnly
// LCOFIP | ReadOnly
word_t RVCSR::read_mip(unsigned int addr, word_t value) {  
    return value & INTER_MASK_M;
}

word_t RVCSR::write_mip(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    return value & ((1 << INTERRUPT_SOFTWARE_S) | (1 << INTERRUPT_TIMER_S));
}

word_t RVCSR::read_mie(unsigned int addr, word_t value) {
    return value & INTER_MASK_M;
}

word_t RVCSR::write_mie(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    return value & INTER_MASK_M;
}

word_t RVCSR::read_sip(unsigned int addr, word_t value) {
    return this->csr[CSR_MIE].value & INTER_MASK_S;
}

word_t RVCSR::write_sip(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    value &= (1 << INTERRUPT_SOFTWARE_S);
    this->csr[CSR_MIP].value &= ~(1 << INTERRUPT_SOFTWARE_S);
    this->csr[CSR_MIP].value |= value;
    return 0;
}

word_t RVCSR::read_sie(unsigned int addr, word_t value) {
    return this->csr[CSR_MIE].value & INTER_MASK_S;
}

word_t RVCSR::write_sie(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    return value & INTER_MASK_S;
}

word_t RVCSR::read_sstatus(unsigned int addr, word_t value) {
    return this->csr[CSR_MSTATUS].value & SSTATUS_MASK;
}

word_t RVCSR::write_sstatus(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    word_t &mstatus = this->csr[CSR_MSTATUS].value;
    mstatus &= ~SSTATUS_MASK;
    mstatus |= value & SSTATUS_MASK;
    return 0;
}

word_t RVCSR::write_stimecmp(unsigned int addr, word_t value, bool &valid) {
    this->csr[CSR_STIMECMP].value = value;
    this->update_stimecmp(this->parentCore);
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
    value &= ~SATP_ASID_MASK; // ASID is WARL
    #ifdef KXEMU_ISA64
    // Check if the mode is valid
    static constexpr word_t validSatpMode64[] = {
        SATP_MODE_BARE,
        SATP_MODE_SV39,
        SATP_MODE_SV48,
        SATP_MODE_SV57
    };
    word_t mode = SATP_MODE(value);
    bool flag = true;
    for (unsigned int i = 0; i < sizeof(validSatpMode64) / sizeof(word_t); i++) {
        if (mode == validSatpMode64[i]) {
            flag = false;
            break;
        }
    }
    if (flag) {
        value &= ~SATP_MODE_MASK; // Set mode to Bare
    }
    #endif
    valid = true;
    return value;
}

word_t RVCSR::read_time(unsigned int addr, word_t value) {
    uint64_t mtime = UPTIME_TO_MTIME(this->get_uptime());
#ifdef KXEMU_ISA32
    csr[CSR_TIMEH].value = mtime >> 32;
#endif
    return mtime;
}


