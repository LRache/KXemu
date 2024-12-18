#include "isa/riscv32/csr.h"
#include "isa/word.h"
#include "macro.h"
#include "isa/riscv32/csr-flag.h"

#include <cstddef>

void RV32CSR::init() {
    // misa
    // Compressed Extension and Integer Multiply/Divide extension
    CSR(0x301, MISA_C | MISA_E | MISA_I | MISA_M);
}

void RV32CSR::add_csr(word_t addr, )

word_t RV32CSR::read_csr(word_t addr, bool &success) {
    auto iter = this->csr.find(addr);
    if (iter == this->csr.end()) {
        success = false;
        return 0;
    }
    success = true;
    word_t value;
    if (likely(iter->second.readFunc == nullptr)) {
        value = iter->second.value;
    } else {
        value = (this->*(iter->second.readFunc))(addr, iter->second.value);
    }
    return value;
}

void RV32CSR::write_csr(word_t addr, word_t value, bool &success) {
    auto iter = this->csr.find(addr);
    if (iter == this->csr.end()) {
        success = false;
        return;
    }
    success = true;
    if (unlikely(iter->second.writeFunc != nullptr)) {
        value = (this->*(iter->second.writeFunc))(addr, value);
    }
    iter->second.value = value;
}
