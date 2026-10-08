#include "cpu/riscv/csr.hpp"
#include "cpu/riscv/csr-field.hpp"
#include "cpu/riscv/def.hpp"
#include "cpu/word.hpp"
#include "log.h"
#include "debug.h"
#include "macro.h"
#include "config/config.h"

#include <cstdint>

using namespace kxemu::cpu;

void RVCSR::init(unsigned int hartId, std::function<uint64_t()> get_uptime) {
    this->resetHartId = hartId;

    this->get_uptime = get_uptime;
}

void RVCSR::reset() {
    const auto lock = this->lock_pending();
    static_cast<RVCSRState &>(*this) = RVCSRState{};
    this->mhartid = this->resetHartId;
    this->set_pending(lock, 0); // Refresh pendingHint.
    this->pmpCfgCount = 0;
}

void RVCSR::reload_pmpcfg() {
    pmpCfgCount = 0;
    for (unsigned int index = 0; index < 64; ++index) {
        const auto &cfg = this->pmpcfg[index];

        auto &pmpConfig = pmpCfgArray[pmpCfgCount];
        pmpConfig.r = cfg.r();
        pmpConfig.w = cfg.w();
        pmpConfig.x = cfg.x();

        switch (cfg.a()) {
            case csr::PMPCfgItem::OFF: break;
            case csr::PMPCfgItem::TOR: {
                if (index == 0) {
                    pmpConfig.start = 0;
                    pmpConfig.end = this->pmpaddr[0] << 2;
                    this->pmpCfgCount++;
                } else {
                    word_t start = this->pmpaddr[index - 1] << 2;
                    word_t end = this->pmpaddr[index] << 2;
                    if (start <= end) {
                        pmpConfig.start = start;
                        pmpConfig.end = end;
                        this->pmpCfgCount++;
                    }
                }
            } break;

            case csr::PMPCfgItem::NA4: {
                pmpConfig.start = this->pmpaddr[index] << 2;
                pmpConfig.end = pmpConfig.start + 4;
                this->pmpCfgCount++;
            } break;

            case csr::PMPCfgItem::NAPOT: {
                uint64_t pmpaddr = this->pmpaddr[index];
                unsigned int lowestZeroPos = __builtin_ctzll(~pmpaddr);
                uint64_t length = 8ULL << lowestZeroPos;
                pmpConfig.start = (pmpaddr & ~((1ULL << lowestZeroPos) - 1));
                pmpConfig.end = pmpConfig.start + length;
                this->pmpCfgCount++;
            } break;
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
