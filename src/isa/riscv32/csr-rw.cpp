#include "isa/riscv32/csr.h"
#include "isa/riscv32/csr-def.h"
#include "isa/word.h"

word_t RV32CSR::read_misa(unsigned int addr, word_t value) {
    return value;
}

word_t RV32CSR::write_misa(unsigned int addr, word_t value, bool &valid) {
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

word_t RV32CSR::read_sip(unsigned int addr, word_t value) {
    return this->csr[CSR_MIDELEG].value & this->csr[CSR_MIE].value;
}

word_t RV32CSR::write_sip(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    if (value == 2) {
        this->csr[CSR_MIP].value |= 2;
    }
    return 0;
}

word_t RV32CSR::read_sstatus(unsigned int addr, word_t value) {
    return this->csr[CSR_MSTATUS].value & SSTATUS_MASK;
}

word_t RV32CSR::write_sstatus(unsigned int addr, word_t value, bool &valid) {
    valid = true;
    word_t &mstatus = this->csr[CSR_MSTATUS].value;
    mstatus &= ~SSTATUS_MASK;
    mstatus |= value & SSTATUS_MASK;
    return 0;
}
