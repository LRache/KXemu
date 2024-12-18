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
