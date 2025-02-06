#include "cpu/riscv/csr.h"
#include "cpu/riscv/def.h"
#include "log.h"
#include "debug.h"
#include "macro.h"
#include "config/config.h"
#include "isa/word.h"
#include <functional>

#define MVENDORID 0x584b5343 // "CSKX" in little-endian
#define MARCHID   0x00CAFFEE // Our architecture ID

using namespace kxemu::cpu;

RVCSR::RVCSR() {
    pmpCfgCount = 0;

    // Machine Information Registers
    add_csr(CSR_MVENDORID, MVENDORID, nullptr, nullptr); // mvendorid, Not implemented
    add_csr(CSR_MARCHID  , MARCHID, nullptr, nullptr); // marchid, Not implemented
    add_csr(CSR_MIMPID ); // mimpid, Not implemented
    add_csr(CSR_MHARTID); // mhartid
    add_csr(CSR_MCFGPTR); // mconfigptr, Not implemented

    // Machine Trap Setup
    add_csr(CSR_MSTATUS); // Not implemented
    add_csr(CSR_MISA, MISA_C | MISA_M, &RVCSR::read_misa, &RVCSR::write_misa); // misa
    add_csr(CSR_MEDELEG); // medeleg
    add_csr(CSR_MIDELEG); // mideleg
    add_csr(CSR_MIE    ); // mie
    add_csr(CSR_MTVEC  ); // mtvec
    add_csr(CSR_MCNTEN ); // mcounteren, Not implemented
#ifdef KXEMU_ISA32
    add_csr(CSR_MSTATUSH); // mstatush, Not implemented
    add_csr(CSR_MEDELEGH); // medelegh
#endif

    // Machine Trap Handling
    add_csr(CSR_MSCRATCH); // mscratch
    add_csr(CSR_MEPC    ); // mepc
    add_csr(CSR_MCAUSE  ); // mcause
    add_csr(CSR_MTVAL   ); // mtval
    add_csr(CSR_MIP     ); // mip
    add_csr(CSR_MTINST  ); // mtinst, Not implemented
    add_csr(CSR_MTVAL2  ); // mtval2, Not implemented

    // Machine Configuration
    add_csr(CSR_MENVCFG); // menvcfg, Not implemented 
    add_csr(CSR_MSECCFG); // mseccfg, Not implemented
#ifdef KXEMU_ISA32
    add_csr(CSR_MENVCFGH); // menvcfgh, Not implemented
    add_csr(CSR_MSECCFGH); // mseccfgh, Not implemented
#endif

    // Physical Memory Protection
    for (int i = 0; i < 16; i += 2) {
        add_csr(CSR_PMPCFG0 + i, 0, nullptr, &RVCSR::write_pmpcfg);
    }
#ifdef KXEMU_ISA32
    for (int i = 1; i < 16; i += 2) {
        add_csr(CSR_PMPCFG0 + i, 0, nullptr, &RVCSR::write_pmpcfg);
    }
#endif
    for (int i = 0; i < 64; i++) {
        add_csr(CSR_PMPADDR0 + i, 0, nullptr, &RVCSR::write_pmpaddr);
    }

    // Supervisor Trap Setup
    add_csr(CSR_SSTATUS); // sstatus
    add_csr(CSR_SIE    , 0, &RVCSR::read_sie, &RVCSR::write_sie); // sie
    add_csr(CSR_STVEC  ); // stvec

    // Supervisor Trap Handling
    add_csr(CSR_SSCRATCH, 0, nullptr, nullptr); // sscratch
    add_csr(CSR_SEPC    , 0, nullptr, nullptr); // sepc
    add_csr(CSR_SCAUSE  , 0, nullptr, nullptr); // scause
    add_csr(CSR_STVAL   , 0, nullptr, nullptr); // stval
    add_csr(CSR_SIP     , 0, &RVCSR::read_sip, &RVCSR::write_sip); // sip
    add_csr(CSR_STIMECMP, 0, nullptr, &RVCSR::write_stimecmp); // stimecmp
#ifdef KXEMU_ISA32
    add_csr(CSR_STIMECMPH, 0, nullptr, nullptr); // stimecmph
#endif
    
    // Supervisor Protection and Translation
    add_csr(CSR_SATP, 0, nullptr, &RVCSR::write_satp); // satp

    // Uprivileged Counter/Timers
    add_csr(CSR_CYCLE, 0, nullptr, nullptr); // cycle, Not implemented
    add_csr(CSR_TIME , 0, &RVCSR::read_time, nullptr); // time
#ifdef KXEMU_ISA32
    add_csr(CSR_CYCLEH, 0, nullptr, nullptr); // cycleh, Not implemented
    add_csr(CSR_TIMEH, 0, nullptr, nullptr); // timeh
#endif
}

void RVCSR::init(unsigned int hartId, std::function<uint64_t()> get_uptime) {    
    this->csr[CSR_MHARTID].resetValue = hartId; // mhartid

    this->get_uptime = get_uptime;
}

void RVCSR::init_callbacks(callback_t update_stimecmp) {
    this->update_stimecmp = update_stimecmp;
}

void RVCSR::reset() {
    for (auto &iter : this->csr) {
        iter.second.value = iter.second.resetValue;
    }

    pmpCfgCount = 0;
}

void RVCSR::add_csr(unsigned int addr, word_t resetValue, csr_rw_func_t readFunc, csr_rw_func_t writeFunc) {
    SELF_PROTECT(this->csr.find(addr) == this->csr.end(), "CSR 0x%03x already exists", addr);
    this->csr[addr] = {readFunc, writeFunc, 0, resetValue};
}

void RVCSR::reload_pmpcfg() {
    pmpCfgCount = 0;
    unsigned int index = 0;
    #ifdef KXEMU_ISA64
    for (unsigned int i = 0; i < 16; i += 2) {
        for (unsigned int j = 0; j < 8; j++) {
    #else
    for (unsigned int i = 0; i < 16; i++) {
        for (unsigned int j = 0; j < 4; j++) {
    #endif
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
    DEBUG("Reload PMP config, count=%d", this->pmpCfgCount);
    DEBUG("PMP config: " FMT_WORD ", " FMT_WORD, this->pmpCfgArray[0].start, this->pmpCfgArray[0].end);
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
    auto pmpConfig = pmp_check(addr, len);
    if (pmpConfig == nullptr) {
        return false;
    }
    // DEBUG("PMP check r, addr=" FMT_WORD ", len=%d, start=" FMT_WORD ", end=" FMT_WORD, addr, len, pmpConfig->start, pmpConfig->end);
    return pmpConfig->r;
}

bool RVCSR::pmp_check_w(word_t addr, int len) {
    auto pmpConfig = pmp_check(addr, len);
    if (pmpConfig == nullptr) {
        return false;
    }
    // DEBUG("PMP check w, addr=" FMT_WORD ", len=%d, start=" FMT_WORD ", end=" FMT_WORD, addr, len, pmpConfig->start, pmpConfig->end);
    return pmpConfig->w;
}

bool RVCSR::pmp_check_x(word_t addr, int len) {
    auto pmpConfig = pmp_check(addr, len);
    if (pmpConfig == nullptr) {
        return false;
    }
    // DEBUG("PMP check x, addr=" FMT_WORD ", len=%d, start=" FMT_WORD ", end=" FMT_WORD, addr, len, pmpConfig->start, pmpConfig->end);
    return pmpConfig->x;
}

word_t *RVCSR::get_csr_ptr(unsigned int addr) {
    auto iter = this->csr.find(addr);
    
    SELF_PROTECT(iter != this->csr.end(), "Access to non-exist CSR 0x%03x", addr);
    // SELF_PROTECT(iter->second.readFunc == nullptr, "Access to CSR 0x%03x with read function", addr);
    // SELF_PROTECT(iter->second.writeFunc == nullptr, "Access to CSR 0x%03x with write function", addr);
    // SELF_PROTECT((addr & CSR_READ_ONLY) != CSR_READ_ONLY, "Access to read-only CSR 0x%03x", addr);
    
    return &iter->second.value;
}

const word_t *RVCSR::get_csr_ptr_readonly(unsigned int addr) const {
    auto iter = this->csr.find(addr);

    SELF_PROTECT(iter != this->csr.end(), "Access to non-exist CSR 0x%03x", addr);
    // SELF_PROTECT(iter->second.readFunc == nullptr, "Access to CSR 0x%03x with read function", addr);
    
    return &iter->second.value;
}

word_t RVCSR::read_csr(unsigned int addr, bool &valid) {
    auto iter = this->csr.find(addr);
    if (iter == this->csr.end()) {
        valid = false;
        return 0;
    }
    valid = true;
    word_t value;
    if (likely(iter->second.readFunc == nullptr)) {
        value = iter->second.value;
    } else {
        value = (this->*(iter->second.readFunc))(addr, iter->second.value, valid);
    }
    return value;
}

word_t RVCSR::read_csr(unsigned int addr) {
    bool valid;
    return read_csr(addr, valid);
}

bool RVCSR::write_csr(unsigned int addr, word_t value) {
    // Whether the destination csr is read-only should be checked in the Core
    SELF_PROTECT((addr & CSR_READ_ONLY) != CSR_READ_ONLY, "Write to read-only CSR 0x%03x", addr);

    auto iter = this->csr.find(addr);
    if (iter == this->csr.end()) {
        WARN("Write to non-exist CSR 0x%03x", addr);
        return false;
    }

    bool valid = true;
    if (unlikely(iter->second.writeFunc != nullptr)) {
        value = (this->*(iter->second.writeFunc))(addr, value, valid);
    }
    if (valid) {
        iter->second.value = value;
    }
    return valid;
}
