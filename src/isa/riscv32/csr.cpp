#include "isa/riscv32/csr.h"
#include "isa/riscv32/csr-def.h"
#include "isa/word.h"
#include "log.h"
#include "debug.h"
#include "macro.h"

#define INIT_CSR(addr, v) this->csr[addr].value = v
#define INIT_CSR_0(addr) INIT_CSR(addr, 0)

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
    add_csr(0x342, 0x1800, nullptr, nullptr); // mcause
    add_csr(0x343, 0, nullptr, nullptr); // mtval
    add_csr(0x344, 0, nullptr, nullptr); // mip

    // Machine Configuration
    add_csr(0x30A, 0, nullptr, nullptr); // menvcfg, Not implemented
    add_csr(0x31A, 0, nullptr, nullptr); // menvcfgh, Not implemented
    add_csr(0x747, 0, nullptr, nullptr); // mseccfg, Not implemented
    add_csr(0x757, 0, nullptr, nullptr); // mseccfgh, Not implemented

    // Physical Memory Protection
    for (int i = 0; i < 16; i++) {
        add_csr(0x3A0 + i, 0, nullptr, &RV32CSR::write_pmpcfg);
    }
    for (int i = 0; i < 64; i++) {
        add_csr(0x3B0 + i, 0, nullptr, &RV32CSR::write_pmpaddr);
    }

    // Supervisor Trap Setup
    add_csr(0x100, 0, nullptr, nullptr); // sstatus
    add_csr(0x104, 0, nullptr, nullptr); // sie
    add_csr(0x105, 0, nullptr, nullptr); // stvec

    // Supervisor Trap Handling
    add_csr(0x141, 0, nullptr, nullptr); // sepc
    add_csr(0x142, 0, nullptr, nullptr); // scause, Not implemented
    add_csr(0x143, 0, nullptr, nullptr); // stval, Not implemented
    add_csr(0x144, 0, &RV32CSR::read_sip, &RV32CSR::write_sip); // sip
    
    // Supervisor Protection and Translation
    add_csr(0x180, 0, nullptr, nullptr); // satp, Not implemented
}

void RV32CSR::reset() {
    INIT_CSR(0xF11, 0);
    INIT_CSR(0xF12, 0);
    INIT_CSR(0xF13, 0);
    INIT_CSR(0xF14, 0);
    INIT_CSR(0xF15, 0);

    INIT_CSR(0x300, 0);
    INIT_CSR(0x301, MISA_C | MISA_M);
    INIT_CSR(0x302, 0);
    INIT_CSR(0x303, 0);
    INIT_CSR(0x304, 0);
    INIT_CSR(0x305, 0);
    INIT_CSR(0x310, 0);
    INIT_CSR(0x312, 0);

    INIT_CSR(0x340, 0);
    INIT_CSR(0x341, 0);
    INIT_CSR(0x342, 0x1800);
    INIT_CSR(0x343, 0);
    INIT_CSR(0x344, 0);

    INIT_CSR(0x30A, 0);
    INIT_CSR(0x31A, 0);
    INIT_CSR(0x747, 0);
    INIT_CSR(0x757, 0);

    for (int i = 0; i < 16; i++) {
        INIT_CSR(0x3A0 + i, 0);
    }
    for (int i = 0; i < 64; i++) {
        INIT_CSR(0x3B0 + i, 0);
    }

    INIT_CSR(0x100, 0);
    INIT_CSR(0x104, 0);
    INIT_CSR(0x105, 0);

    INIT_CSR(0x141, 0);
    INIT_CSR(0x142, 0);
    INIT_CSR(0x143, 0);
    INIT_CSR(0x144, 0);

    INIT_CSR(0x180, 0);

    pmpCfgCount = 0;
}

void RV32CSR::add_csr(word_t addr, word_t init, csr_read_fun_t readFunc, csr_write_fun_t writeFunc) {
    this->csr[addr] = {readFunc, writeFunc, init};
}

void RV32CSR::reload_pmpcfg() {
    pmpCfgCount = 0;
    unsigned int index = 0;
    for (unsigned int i = 0; i < 16; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            word_t cfg = this->csr[CSR_PMPCFG0 + i].value >> (j * 8);
            word_t a = ((cfg & PMPCFG_A_MASK) >> PMPCFG_A_OFF);
            
            auto &pmpConfig = pmpCfgArray[pmpCfgCount];
            pmpConfig.r = cfg & PMPCFG_R_MASK;
            pmpConfig.w = cfg & PMPCFG_W_MASK;
            pmpConfig.x = cfg & PMPCFG_X_MASK;
            
            if (a == PMPCONFIG_A_TOR) {
                if (index == 0) {
                    pmpConfig.start = 0;
                    pmpConfig.end = csr[CSR_PMPADDR0].value << 2;
                    this->pmpCfgCount++;
                } else {
                    word_t start = csr[CSR_PMPADDR0 + index - 1].value << 2;
                    word_t end = csr[CSR_PMPADDR0 + index].value << 2;
                    if (start <= end) {
                        pmpConfig.start = start;
                        pmpConfig.end = end;
                        this->pmpCfgCount++;
                    }
                }
            } else if (a == PMPCONFIG_A_NA4) {
                pmpConfig.start = csr[CSR_PMPADDR0 + index].value << 2;
                pmpConfig.end = pmpConfig.start + 4;
                this->pmpCfgCount++;
            } else if (a == PMPCONFIG_A_NAPOT) {
                pmpConfig.start = csr[CSR_PMPADDR0 + index].value << 2;
                word_t addr = csr[CSR_PMPADDR0 + index].value;
                word_t length = 4;
                for (int k = 0; k < 32; k++) {
                    if (addr & 1){
                        length <<= 1;
                        addr >>= 1;
                    }
                    else break;
                }
                pmpConfig.end = pmpConfig.start + length;
                this->pmpCfgCount++;
            }
            index ++;
        }
    }
}

RV32CSR::PMPCfg *RV32CSR::pmp_check(word_t addr, int len) {
    for (unsigned int i = 0; i < this->pmpCfgCount; i++) {
        auto &pmpConfig = this->pmpCfgArray[i];
        if (addr >= pmpConfig.start && addr + len < pmpConfig.end) {
            return &pmpConfig;
        }
    }
    return nullptr;
}

bool RV32CSR::pmp_check_r(word_t addr, int len) {
    auto pmpConfig = pmp_check(addr, len);
    if (pmpConfig == nullptr) {
        return false;
    }
    return pmpConfig->r;
}

bool RV32CSR::pmp_check_w(word_t addr, int len) {
    auto pmpConfig = pmp_check(addr, len);
    if (pmpConfig == nullptr) {
        return false;
    }
    return pmpConfig->w;
}

bool RV32CSR::pmp_check_x(word_t addr, int len) {
    auto pmpConfig = pmp_check(addr, len);
    if (pmpConfig == nullptr) {
        return false;
    }
    return pmpConfig->x;
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
