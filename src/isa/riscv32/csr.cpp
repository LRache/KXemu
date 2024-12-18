#include "isa/riscv32/csr.h"
#include "isa/riscv32/csr-def.h"
#include "isa/word.h"
#include "log.h"
#include "macro.h"

#include <cstddef>

void RV32CSR::init(unsigned int hartId) {
    // misa
    // Compressed Extension and Integer Multiply/Divide extension
    add_csr(0x301, MISA_C | MISA_M, &RV32CSR::read_misa, &RV32CSR::write_misa);
    
    add_csr(0xf11, 0, nullptr, nullptr); // mvendorid, Not implemented
    add_csr(0xf12, 0, nullptr, nullptr); // marchid, Not implemented
    add_csr(0xf13, 0, nullptr, nullptr); // mimpid, Not implemented
    add_csr(0xf14, hartId, nullptr, nullptr); // mhartid, Not implemented

    add_csr(0x300, 0, nullptr, nullptr); // mstatus,  Not implemented
    add_csr(0x310, 0, nullptr, nullptr); // mstatush, Not implemented
}

void RV32CSR::add_csr(word_t addr, word_t init, csr_read_fun_t readFunc, csr_write_fun_t writeFunc) {
    this->csr[addr] = {readFunc, writeFunc, init};
}

word_t RV32CSR::get_csr(unsigned int addr, bool &success) {
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

void RV32CSR::set_csr(unsigned int addr, word_t value, bool &success) {
    // Whether the destination csr is read-only should be checked in the Core
    if (unlikely((addr & CSR_READ_ONLY) == CSR_READ_ONLY)) {
        PANIC("Write to read-only CSR 0x%03x", addr);
        return;
    }

    auto iter = this->csr.find(addr);
    if (iter == this->csr.end()) {
        success = false;
        return;
    }
    if (unlikely(iter->second.writeFunc != nullptr)) {
        value = (this->*(iter->second.writeFunc))(addr, value, success);
    }
    iter->second.value = value;
}
