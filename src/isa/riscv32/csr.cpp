#include "isa/riscv32/csr.h"
#include "isa/riscv32/csr-def.h"
#include "isa/word.h"
#include "log.h"
#include "debug.h"
#include "macro.h"


void RV32CSR::init(unsigned int hartId) {    
    add_csr(0xF11, 0, nullptr, nullptr); // mvendorid, Not implemented
    add_csr(0xF12, 0, nullptr, nullptr); // marchid, Not implemented
    add_csr(0xF13, 0, nullptr, nullptr); // mimpid, Not implemented
    add_csr(0xF14, hartId, nullptr, nullptr); // mhartid, Not implemented
    add_csr(0xF15, 0, nullptr, nullptr); // msconfigptr, Not implemented

    // Machine Trap Setup
    add_csr(0x300, 0, nullptr, nullptr); // mstatus,  Not implemented
    add_csr(0x301, MISA_C | MISA_M, &RV32CSR::read_misa, &RV32CSR::write_misa); // misa
    add_csr(0x302, 0, nullptr, nullptr); // medeleg
    add_csr(0x303, 0, nullptr, nullptr); // mideleg
    add_csr(0x304, 0, nullptr, nullptr); // mie
    add_csr(0x305, 0, nullptr, nullptr); // mtvec
    add_csr(0x310, 0, nullptr, nullptr); // mstatush, Not implemented
    add_csr(0x312, 0, nullptr, nullptr); // medelegh

    // Machine Trap Handling
    add_csr(0x340, 0, nullptr, nullptr); // mscratch
    add_csr(0x341, 0, nullptr, nullptr); // mepc
    add_csr(0x342, 0, nullptr, nullptr); // mcause
    add_csr(0x343, 0, nullptr, nullptr); // mtval
    add_csr(0x344, 0, nullptr, nullptr); // mip

    // Machine Configuration
    add_csr(0x30A, 0, nullptr, nullptr); // menvcfg, Not implemented
    add_csr(0x31A, 0, nullptr, nullptr); // menvcfgh, Not implemented
    add_csr(0x747, 0, nullptr, nullptr); // mseccfg, Not implemented
    add_csr(0x757, 0, nullptr, nullptr); // mseccfgh, Not implemented

    // Supervisor Trap Setup
    add_csr(0x100, 0, nullptr, nullptr); // sstatus, Not implemented
    add_csr(0x104, 0, nullptr, nullptr); // sie
    add_csr(0x105, 0, nullptr, nullptr); // stvec, Not implemented

    // Supervisor Trap Handling
    add_csr(0x141, 0, nullptr, nullptr); // sepc
    add_csr(0x142, 0, nullptr, nullptr); // scause, Not implemented
    add_csr(0x143, 0, nullptr, nullptr); // stval, Not implemented
    add_csr(0x144, 0, &RV32CSR::read_sip, &RV32CSR::write_sip); // sip
    
    // Supervisor Protection and Translation
    add_csr(0x180, 0, nullptr, nullptr); // satp, Not implemented
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

word_t *RV32CSR::get_csr_ptr(unsigned int addr) {
    auto iter = this->csr.find(addr);
    
    SELF_PROTECT(iter != this->csr.end(), "Access to non-exist CSR 0x%03x", addr);
    SELF_PROTECT(iter->second.readFunc == nullptr, "Access to CSR 0x%03x with read function", addr);
    SELF_PROTECT(iter->second.writeFunc == nullptr, "Access to CSR 0x%03x with write function", addr);
    SELF_PROTECT((addr & CSR_READ_ONLY) != CSR_READ_ONLY, "Access to read-only CSR 0x%03x", addr);
    
    return &iter->second.value;
}

const word_t *RV32CSR::get_csr_ptr_readonly(unsigned int addr) const {
    auto iter = this->csr.find(addr);

    SELF_PROTECT(iter != this->csr.end(), "Access to non-exist CSR 0x%03x", addr);
    SELF_PROTECT(iter->second.readFunc == nullptr, "Access to CSR 0x%03x with read function", addr);
    
    return &iter->second.value;
}

void RV32CSR::set_csr(unsigned int addr, word_t value, bool &success) {
    // Whether the destination csr is read-only should be checked in the Core
    SELF_PROTECT((addr & CSR_READ_ONLY) != CSR_READ_ONLY, "Write to read-only CSR 0x%03x", addr);

    auto iter = this->csr.find(addr);
    if (iter == this->csr.end()) {
        WARN("Write to non-exist CSR 0x%03x", addr);
        success = false;
        return;
    }
    if (unlikely(iter->second.writeFunc != nullptr)) {
        value = (this->*(iter->second.writeFunc))(addr, value, success);
    }
    success = true;
    iter->second.value = value;
}
