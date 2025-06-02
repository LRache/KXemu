#include "cpu/riscv/csr.h"
#include "cpu/riscv/csr-field.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"
#include "log.h"
#include "debug.h"
#include "macro.h"
#include "config/config.h"
#include <cstdint>

using namespace kxemu::cpu;

RVCSR::RVCSR() {
    pmpCfgCount = 0;

    // Machine Information Registers
    add_csr(CSRAddr::MVENDORID, nullptr, nullptr, KXEMU_VENDORID); // mvendorid
    add_csr(CSRAddr::MARCHID  , nullptr, nullptr, KXEMU_ARCHID  ); // marchid
    add_csr(CSRAddr::MIMPID ); // mimpid, Not implemented
    add_csr(CSRAddr::MHARTID); // mhartid
    add_csr(CSRAddr::MCFGPTR); // mconfigptr, Not implemented

    // Machine Trap Setup
    add_csr(CSRAddr::MSTATUS, nullptr, nullptr, 0x1800); // Not implemented
    add_csr(CSRAddr::MISA, nullptr, nullptr, csr::MISA()); // misa
    add_csr(CSRAddr::MEDELEG); // medeleg
    add_csr(CSRAddr::MIDELEG); // mideleg
    add_csr(CSRAddr::MIE    ); // mie
    add_csr(CSRAddr::MTVEC  ); // mtvec
    add_csr(CSRAddr::MCNTEN ); // mcounteren, Not implemented
#ifdef KXEMU_ISA32
    add_csr(CSRAddr::MSTATUSH); // mstatush, Not implemented
    add_csr(CSRAddr::MEDELEGH); // medelegh
#endif

    // Machine Trap Handling
    add_csr(CSRAddr::MSCRATCH); // mscratch
    add_csr(CSRAddr::MEPC    ); // mepc
    add_csr(CSRAddr::MCAUSE  ); // mcause
    add_csr(CSRAddr::MTVAL   ); // mtval
    add_csr(CSRAddr::MIP     , nullptr, &RVCSR::write_mip); // mip
    add_csr(CSRAddr::MTINST  ); // mtinst, Not implemented
    add_csr(CSRAddr::MTVAL2  ); // mtval2, Not implemented

    // Machine Configuration
    add_csr(CSRAddr::MENVCFG); // menvcfg, Not implemented 
    add_csr(CSRAddr::MSECCFG); // mseccfg, Not implemented
#ifdef KXEMU_ISA32
    add_csr(CSRAddr::MENVCFGH); // menvcfgh, Not implemented
    add_csr(CSRAddr::MSECCFGH); // mseccfgh, Not implemented
#endif

    // Physical Memory Protection
    for (int i = 0; i < 16; i += 2) {
        add_csr((CSRAddr)(CSRAddr::PMPCFG0 + i), nullptr, &RVCSR::write_pmpcfg);
    }
#ifdef KXEMU_ISA32
    for (int i = 1; i < 16; i += 2) {
        add_csr((CSRAddr)(CSRAddr::PMPCFG0 + i), nullptr, &RVCSR::write_pmpcfg);
    }
#endif
    for (int i = 0; i < 64; i++) {
        add_csr((CSRAddr)(CSRAddr::PMPADDR0 + i), nullptr, &RVCSR::write_pmpaddr);
    }

    // Supervisor Trap Setup
    add_csr(CSRAddr::SSTATUS, &RVCSR::read_sstatus, &RVCSR::write_sstatus); // sstatus
    add_csr(CSRAddr::SIE    , nullptr, &RVCSR::write_sie); // sie
    add_csr(CSRAddr::STINST ); // stvec, Not implemented
    add_csr(CSRAddr::STVEC  ); // stvec
    add_csr(CSRAddr::SCNTEN ); // scounteren

    // Supervisor Trap Handling
    add_csr(CSRAddr::SSCRATCH); // sscratch
    add_csr(CSRAddr::SEPC    ); // sepc
    add_csr(CSRAddr::SCAUSE  ); // scause
    add_csr(CSRAddr::STVAL   ); // stval
    add_csr(CSRAddr::SIP     , nullptr, &RVCSR::write_sip); // sip
    add_csr(CSRAddr::STIMECMP); // stimecmp
#ifdef KXEMU_ISA32
    add_csr(CSRAddr::STIMECMPH); // stimecmph
#endif
    
    // Supervisor Protection and Translation
    add_csr(CSRAddr::SATP, nullptr, &RVCSR::write_satp); // satp

    // Unprivileged Floating-Point CSRs
    add_csr(CSRAddr::FFLAGS, &RVCSR::read_fflags, &RVCSR::write_fflags); // fflags
    add_csr(CSRAddr::FRM   , &RVCSR::read_frm   , &RVCSR::write_frm   ); // frm
    add_csr(CSRAddr::FCSR  ); // fcsr

    // Uprivileged Counter/Timers
    add_csr(CSRAddr::CYCLE); // cycle, Not implemented
    add_csr(CSRAddr::TIME , &RVCSR::read_time); // time
#ifdef KXEMU_ISA32
    add_csr(CSRAddr::CYCLEH); // cycleh, Not implemented
    add_csr(CSRAddr::TIMEH , &RVCSR::read_timeh); // timeh
#endif
}

void RVCSR::init(unsigned int hartId, std::function<uint64_t()> get_uptime) {
    this->csr[CSRAddr::MHARTID].resetValue = hartId; // mhartid

    this->get_uptime = get_uptime;
}

void RVCSR::set_write_callbacks(unsigned int addr, callback_t callback) {
    auto iter = this->csr.find(addr);
    SELF_PROTECT(iter != this->csr.end(), "CSR not found");
    iter->second.writeCallback = callback;
}

void RVCSR::reset() {
    for (auto &iter : this->csr) {
        iter.second.value = iter.second.resetValue;
    }

    pmpCfgCount = 0;
}

void RVCSR::add_csr(CSRAddr addr, csr_read_func_t readFunc, csr_write_func_t writeFunc, word_t resetValue) {
    SELF_PROTECT(this->csr.find(addr) == this->csr.end(), "CSR 0x%03x already exists", addr);
    this->csr[addr] = {readFunc, writeFunc, 0, resetValue, nullptr};
}

void RVCSR::reload_pmpcfg() {
    pmpCfgCount = 0;
    unsigned int index = 0;
    #ifdef KXEMU_ISA64
    for (unsigned int i = 0; i < 16; i += 2) {
        csr::PMPCfg pmpcfg = this->get_csr_value((CSRAddr)(CSRAddr::PMPCFG0 + i));
        for (unsigned int j = 0; j < 8; j++) {
    #else
    for (unsigned int i = 0; i < 16; i++) {
        csr::PMPCfg pmpcfg = this->get_csr_value((CSRAddr)(CSRAddr::PMPCFG0 + i));
        for (unsigned int j = 0; j < 4; j++) {
    #endif
            csr::PMPCfgItem cfg = pmpcfg.item(j);
            
            auto &pmpConfig = pmpCfgArray[pmpCfgCount];
            pmpConfig.r = cfg.r();
            pmpConfig.w = cfg.w();
            pmpConfig.x = cfg.x();

            switch (cfg.a()) {
                case csr::PMPCfgItem::OFF: break;
                case csr::PMPCfgItem::TOR: {
                    if (index == 0) {
                        pmpConfig.start = 0;
                        pmpConfig.end = csr[CSRAddr::PMPADDR0].value << 2;
                        this->pmpCfgCount++;
                    } else {
                        word_t start = csr[CSRAddr::PMPADDR0 + index - 1].value << 2;
                        word_t end = csr[CSRAddr::PMPADDR0 + index].value << 2;
                        if (start <= end) {
                            pmpConfig.start = start;
                            pmpConfig.end = end;
                            this->pmpCfgCount++;
                        }
                    }
                } break;
                
                case csr::PMPCfgItem::NA4: {
                    pmpConfig.start = csr[CSRAddr::PMPADDR0 + index].value << 2;
                    pmpConfig.end = pmpConfig.start + 4;
                    this->pmpCfgCount++;
                } break;
                
                case csr::PMPCfgItem::NAPOT: {
                    uint64_t pmpaddr = this->get_csr_value((CSRAddr)(CSRAddr::PMPADDR0 + index));
                    unsigned int lowestZeroPos = __builtin_ctzll(~pmpaddr);
                    uint64_t length = 8ULL << lowestZeroPos;
                    pmpConfig.start = (pmpaddr & ~((1ULL << lowestZeroPos) - 1));
                    pmpConfig.end = pmpConfig.start + length;
                    this->pmpCfgCount++;
                } break;
            }
            index ++;
        }
    }
}

RVCSR::PMPCfg *RVCSR::pmp_check(word_t addr, int len) {
    for (unsigned int i = 0; i < this->pmpCfgCount; i++) {
        auto &pmpConfig = this->pmpCfgArray[i];
        if (addr >= pmpConfig.start && addr + len < pmpConfig.end) {
            return &pmpConfig;
        }
    }
    return nullptr;
}

bool RVCSR::pmp_check_r(word_t addr, int len) {
    // auto pmpConfig = pmp_check(addr, len);
    // if (pmpConfig == nullptr) {
    //     return false;
    // }
    // return pmpConfig->r;
    return true;
}

bool RVCSR::pmp_check_w(word_t addr, int len) {
    // auto pmpConfig = pmp_check(addr, len);
    // if (pmpConfig == nullptr) {
    //     return false;
    // }
    // return pmpConfig->w;
    return true;
}

bool RVCSR::pmp_check_x(word_t addr, int len) {
    // auto pmpConfig = pmp_check(addr, len);
    // if (pmpConfig == nullptr) {
    //     return false;
    // }
    // return pmpConfig->x;
    return true;
}

word_t *RVCSR::get_csr_ptr(unsigned int addr) {
    auto iter = this->csr.find(addr);
    
    SELF_PROTECT(iter != this->csr.end(), "Access to non-exist CSR 0x%03x", addr);
    // SELF_PROTECT(iter->second.readFunc == nullptr, "Access to CSR 0x%03x with read function", addr);
    // SELF_PROTECT(iter->second.writeFunc == nullptr, "Access to CSR 0x%03x with write function", addr);
    // SELF_PROTECT((addr & CSRAddr::READ_ONLY) != CSRAddr::READ_ONLY, "Access to read-only CSR 0x%03x", addr);
    
    return &iter->second.value;
}

const word_t *RVCSR::get_csr_ptr_readonly(unsigned int addr) const {
    auto iter = this->csr.find(addr);

    SELF_PROTECT(iter != this->csr.end(), "Access to non-exist CSR 0x%03x", addr);
    
    return &iter->second.value;
}

word_t RVCSR::read_csr(unsigned int addr, bool &valid) {
    // if (csr::csr_privilege_level(addr) > this->privMode) {
    //     // If the CSR is not accessible in the current privilege mode, we should not read it
    //     WARN("Read from CSR 0x%03x in privilege mode %d", addr, this->privMode);
    //     valid = false;
    //     return 0;
    // }
    
    auto iter = this->csr.find(addr);
    if (iter == this->csr.end()) {
        valid = false;
        return 0;
    }
    
    word_t value = -1;
    if (likely(iter->second.readFunc == nullptr)) {
        value = iter->second.value;
        valid = true;
    } else {
        auto v = (this->*(iter->second.readFunc))(addr);
        valid = v.has_value();
        if (valid) {
            value = v.value();
        }
    }
    
    return value;
}

word_t RVCSR::read_csr(unsigned int addr) {
    bool valid;
    return read_csr(addr, valid);
}

bool RVCSR::write_csr(unsigned int addr, word_t value) {
    // Whether the destination csr is read-only should be checked in the Core
    SELF_PROTECT(csr_read_only(addr), "Write to read-only CSR 0x%03x", addr);

    // if (csr::csr_privilege_level(addr) > this->privMode) {
    //     // If the CSR is not accessible in the current privilege mode, we should not write to it
    //     WARN("Write to CSR 0x%03x in privilege mode %d", addr, this->privMode);
    //     return false;
    // }

    auto iter = this->csr.find(addr);
    if (iter == this->csr.end()) {
        WARN("Write to non-exist CSR 0x%03x", addr);
        return false;
    }

    bool valid;
    if (unlikely(iter->second.writeFunc != nullptr)) {
        valid = (this->*(iter->second.writeFunc))(addr, value);    
    } else {      
        iter->second.value = value;
        valid = true;
    }
    if (valid) {
        if (iter->second.writeCallback != nullptr) {
            iter->second.writeCallback();
        }
    }
    return valid;
}
