#include "cpu/riscv/csr.h"
#include "cpu/riscv/def.h"
#include "cpu/riscv/namespace.h"
#include "cpu/word.h"
#include "log.h"
#include "debug.h"
#include "macro.h"
#include "config/config.h"

using namespace kxemu::cpu;

#define KXEMU_MVENDORID 0x584b5343 // "CSKX" in little-endian
#define KXEMU_MARCHID   0x00CAFFEE // Our architecture ID

static constexpr word_t misaFlag = MISAFlag::A | MISAFlag::C | MISAFlag::D | MISAFlag::E | MISAFlag::F | MISAFlag::I | MISAFlag::M | MISAFlag::S | MISAFlag::U;

RVCSR::RVCSR() {
    pmpCfgCount = 0;

    // Machine Information Registers
    add_csr(CSRAddr::MVENDORID, KXEMU_MVENDORID); // mvendorid
    add_csr(CSRAddr::MARCHID  , KXEMU_MARCHID  ); // marchid
    add_csr(CSRAddr::MIMPID ); // mimpid, Not implemented
    add_csr(CSRAddr::MHARTID); // mhartid
    add_csr(CSRAddr::MCFGPTR); // mconfigptr, Not implemented

    // Machine Trap Setup
    add_csr(CSRAddr::MSTATUS); // Not implemented
    add_csr(CSRAddr::MISA, misaFlag, &RVCSR::read_misa, &RVCSR::write_misa); // misa
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
    add_csr(CSRAddr::MIP     ); // mip
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
        add_csr(CSRAddr::PMPCFG0 + i, 0, nullptr, &RVCSR::write_pmpcfg);
    }
#ifdef KXEMU_ISA32
    for (int i = 1; i < 16; i += 2) {
        add_csr(CSRAddr::PMPCFG0 + i, 0, nullptr, &RVCSR::write_pmpcfg);
    }
#endif
    for (int i = 0; i < 64; i++) {
        add_csr(CSRAddr::PMPADDR0 + i, 0, nullptr, &RVCSR::write_pmpaddr);
    }

    // Supervisor Trap Setup
    add_csr(CSRAddr::SSTATUS, 0, &RVCSR::read_sstatus, &RVCSR::write_sstatus); // sstatus
    add_csr(CSRAddr::SIE    , 0, &RVCSR::read_sie, &RVCSR::write_sie); // sie
    add_csr(CSRAddr::STVEC  ); // stvec

    // Supervisor Trap Handling
    add_csr(CSRAddr::SSCRATCH); // sscratch
    add_csr(CSRAddr::SEPC    ); // sepc
    add_csr(CSRAddr::SCAUSE  ); // scause
    add_csr(CSRAddr::STVAL   ); // stval
    add_csr(CSRAddr::SIP     , 0, &RVCSR::read_sip, &RVCSR::write_sip); // sip
    add_csr(CSRAddr::STIMECMP, 0, nullptr, &RVCSR::write_stimecmp); // stimecmp
#ifdef KXEMU_ISA32
    add_csr(CSRAddr::STIMECMPH, 0, nullptr, nullptr); // stimecmph
#endif
    
    // Supervisor Protection and Translation
    add_csr(CSRAddr::SATP, 0, nullptr, &RVCSR::write_satp); // satp

    // Unprivileged Floating-Point CSRs
    add_csr(CSRAddr::FFLAGS); // fflags, Not implemented
    add_csr(CSRAddr::FRM   ); // frm, Not implemented
    add_csr(CSRAddr::FCSR  ); // fcsr, Not implemented

    // Uprivileged Counter/Timers
    add_csr(CSRAddr::CYCLE); // cycle, Not implemented
    add_csr(CSRAddr::TIME , 0, &RVCSR::read_time, nullptr); // time
#ifdef KXEMU_ISA32
    add_csr(CSRAddr::CYCLEH); // cycleh, Not implemented
    add_csr(CSRAddr::TIMEH ); // timeh
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

void RVCSR::add_csr(unsigned int addr, word_t resetValue, csr_rw_func_t readFunc, csr_rw_func_t writeFunc) {
    SELF_PROTECT(this->csr.find(addr) == this->csr.end(), "CSR 0x%03x already exists", addr);
    this->csr[addr] = {readFunc, writeFunc, 0, resetValue, nullptr};
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
            word_t cfg = this->csr[CSRAddr::PMPCFG0 + i].value >> (j * 8);
            word_t a = ((cfg & PMPCFG_A_MASK) >> PMPCFG_A_OFF);
            
            auto &pmpConfig = pmpCfgArray[pmpCfgCount];
            pmpConfig.r = cfg & PMPCFG_R_MASK;
            pmpConfig.w = cfg & PMPCFG_W_MASK;
            pmpConfig.x = cfg & PMPCFG_X_MASK;

            switch (a) {
                case PMPConfigAFlag::OFF: break;
                case PMPConfigAFlag::TOR: {
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
                
                case PMPConfigAFlag::NA4: {
                    pmpConfig.start = csr[CSRAddr::PMPADDR0 + index].value << 2;
                    pmpConfig.end = pmpConfig.start + 4;
                    this->pmpCfgCount++;
                } break;
                
                case PMPConfigAFlag::NAPOT: {
                    pmpConfig.start = csr[CSRAddr::PMPADDR0 + index].value << 2;
                    word_t addr = csr[CSRAddr::PMPADDR0 + index].value;
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
                } break;
            }
            
            // if (a == PMPConfigAFlag::TOR) {
            //     if (index == 0) {
            //         pmpConfig.start = 0;
            //         pmpConfig.end = csr[CSRAddr::PMPADDR0].value << 2;
            //         this->pmpCfgCount++;
            //     } else {
            //         word_t start = csr[CSRAddr::PMPADDR0 + index - 1].value << 2;
            //         word_t end = csr[CSRAddr::PMPADDR0 + index].value << 2;
            //         if (start <= end) {
            //             pmpConfig.start = start;
            //             pmpConfig.end = end;
            //             this->pmpCfgCount++;
            //         }
            //     }
            // } else if (a == PMPCONFIG_A_NA4) {
            //     pmpConfig.start = csr[CSRAddr::PMPADDR0 + index].value << 2;
            //     pmpConfig.end = pmpConfig.start + 4;
            //     this->pmpCfgCount++;
            // } else if (a == PMPCONFIG_A_NAPOT) {
            //     pmpConfig.start = csr[CSRAddr::PMPADDR0 + index].value << 2;
            //     word_t addr = csr[CSRAddr::PMPADDR0 + index].value;
            //     word_t length = 4;
            //     for (int k = 0; k < 32; k++) {
            //         if (addr & 1){
            //             length <<= 1;
            //             addr >>= 1;
            //         }
            //         else break;
            //     }
            //     pmpConfig.end = pmpConfig.start + length;
            //     this->pmpCfgCount++;
            // }
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
    auto pmpConfig = pmp_check(addr, len);
    if (pmpConfig == nullptr) {
        return false;
    }
    return pmpConfig->r;
}

bool RVCSR::pmp_check_w(word_t addr, int len) {
    auto pmpConfig = pmp_check(addr, len);
    if (pmpConfig == nullptr) {
        return false;
    }
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
    // SELF_PROTECT((addr & CSRAddr::READ_ONLY) != CSRAddr::READ_ONLY, "Access to read-only CSR 0x%03x", addr);
    
    return &iter->second.value;
}

const word_t *RVCSR::get_csr_ptr_readonly(unsigned int addr) const {
    auto iter = this->csr.find(addr);

    SELF_PROTECT(iter != this->csr.end(), "Access to non-exist CSR 0x%03x", addr);
    
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
        if (iter->second.writeCallback != nullptr) {
            iter->second.writeCallback();
        }
    }
    return valid;
}
