#include "cpu/riscv/core.hpp"
#include "cpu/riscv/aclint.hpp"
#include "cpu/word.hpp"
#include "device/bus.hpp"
#include "log.h"
#include "word.h"

#include <cstdint>
#include <cstring>
#include <optional>

using namespace kxemu::cpu;

RVCore::RVCore() {
    this->vaddr_translate_func = &RVCore::vaddr_translate_bare;

    this->deviceMtx = nullptr;
}

void RVCore::init(unsigned int coreID, device::Bus *bus, device::AClint *alint, device::PLIC *plic) {
    this->coreID = coreID;
    this->bus = bus;
    this->aclint = alint;
    this->plic = plic;
    this->state = IDLE;

    this->init_csr();
}

void RVCore::reset(word_t entry) {
    {
        const auto lock = this->csr.lock_pending();
        this->wakePending = false;
        this->stopping.store(false, std::memory_order_release);
    }
    this->pc = entry;
    this->state = RUNNING;
    this->privMode = PrivMode::MACHINE;
    
    this->csr.reset();

    std::memset(this->gpr, 0, sizeof(this->gpr));
    this->gpr[10] = this->coreID;

    #ifdef CONFIG_ENABLE_ICACHE
    this->icache_fence();
    #endif
    this->tlb_fence();
}

uint64_t RVCore::get_uptime() {
    return this->aclint->get_uptime();
}

bool RVCore::is_error() {
    return this->state == ERROR;
}

bool RVCore::is_break() {
    return this->state == BREAKPOINT;
}

bool RVCore::is_running() {
    return this->state == RUNNING;
}

bool RVCore::is_halt() {
    return this->state == HALT;
}

word_t RVCore::get_pc() {
    return this->pc;
}

void RVCore::set_pc(word_t pc) {
    this->pc = pc;
}

word_t RVCore::get_gpr(unsigned int index) {
    return gpr[index];
}

void RVCore::set_gpr(unsigned int index, word_t value) {
    DEBUG("SET gpr[%u] = " FMT_WORD ", pc = " FMT_WORD, index, value, pc);
    this->gpr[index] = value;
}

static inline const char *gprAlias[] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

static inline const char *gprNames[] = {
    "x0" , "x1" , "x2" , "x3" , "x4" , "x5" , "x6" , "x7" ,
    "x8" , "x9" , "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
    "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"
};

static constexpr struct {
    const char *name;
    unsigned int addr;
} csrNames[] = {
    {"mstatus" , CSRAddr::MSTATUS }, {"misa"    , CSRAddr::MISA    },
    {"medeleg" , CSRAddr::MEDELEG }, {"mideleg" , CSRAddr::MIDELEG },
    {"mie"     , CSRAddr::MIE     }, {"mtvec"   , CSRAddr::MTVEC   },
    {"mcnten"  , CSRAddr::MCNTEN  }, {"mscratch", CSRAddr::MSCRATCH},
    {"mepc"    , CSRAddr::MEPC    }, {"mcause"  , CSRAddr::MCAUSE  },
    {"mtval"   , CSRAddr::MTVAL   }, {"mip"     , CSRAddr::MIP     },
    {"mtinst"  , CSRAddr::MTINST  }, {"mtval2"  , CSRAddr::MTVAL2  },
    {"pmpcfg0" , CSRAddr::PMPCFG0 },
    {"sstatus" , CSRAddr::SSTATUS }, {"sie"     , CSRAddr::SIE     },
    {"stvec"   , CSRAddr::STVEC   }, {"scnten"  , CSRAddr::SCNTEN  },
    {"sscratch", CSRAddr::SSCRATCH}, {"sepc"    , CSRAddr::SEPC    },
    {"scause"  , CSRAddr::SCAUSE  }, {"stval"   , CSRAddr::STVAL   },
    {"sip"     , CSRAddr::SIP     }, {"stimecmp", CSRAddr::STIMECMP},
    {"satp"    , CSRAddr::SATP    },
};

static std::optional<unsigned int> csr_addr_by_name(const std::string &name) {
    for (const auto &entry : csrNames) {
        if (name == entry.name) return entry.addr;
    }
    return std::nullopt;
}

std::optional<word_t> RVCore::get_register(const std::string &name) {
    for (unsigned int i = 0; i < 32; i++) {
        if (name == gprNames[i]) {
            return this->gpr[i];
        }
    }

    for (unsigned int i = 0; i < 32; i++) {
        if (name == gprAlias[i]) {
            return this->gpr[i];
        }
    }

    if (name == "pc") {
        return this->pc;
    } else if (name == "priv") {
        return this->privMode;
    }
    
    if (const auto addr = csr_addr_by_name(name)) {
        return this->csr.debug_read_csr(*addr);
    }

    return std::nullopt;
}

bool RVCore::set_register(const std::string &name, word_t value) {
    for (unsigned int i = 1; i < 32; i++) {
        if (name == gprNames[i]) {
            this->gpr[i] = value;
            return true;
        }
    }

    for (unsigned int i = 1; i < 32; i++) {
        if (name == gprAlias[i]) {
            this->gpr[i] = value;
            return true;
        }
    }

    if (name == "pc") {
        this->pc = value;
        return true;
    }

    if (const auto addr = csr_addr_by_name(name)) {
        const auto effect = this->csr.debug_write_csr(*addr, value);
        if (!effect.has_value()) return false;
        this->apply_csr_effect(*effect);
        return true;
    }

    return false;
}

word_t RVCore::get_halt_pc() {
    return this->haltPC;
}

word_t RVCore::get_halt_code() {
    return this->haltCode;
}

RVCore::~RVCore() {}
