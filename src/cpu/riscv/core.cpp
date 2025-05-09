#include "cpu/riscv/core.h"
#include "cpu/riscv/aclint.h"
#include "cpu/word.h"
#include "device/bus.h"
#include "log.h"
#include "word.h"

#include <cstdint>
#include <cstring>
#include <optional>

using namespace kxemu::cpu;

RVCore::RVCore() {    
    this->medeleg = this->csr.get_csr_ptr_readonly(CSRAddr::MEDELEG);
    this->mideleg = this->csr.get_csr_ptr_readonly(CSRAddr::MIDELEG);
    this->mie     = this->csr.get_csr_ptr_readonly(CSRAddr::MIE);
    this->mip     = this->csr.get_csr_ptr(CSRAddr::MIP);

    // RV32 only
#ifdef KXEMU_ISA32
    this->medelegh= this->csr.get_csr_ptr_readonly(CSRAddr::MEDELEGH);
#endif

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
    this->pc = entry;
    this->state = RUNNING;
    this->privMode = PrivMode::MACHINE;
    
    this->csr.reset();

    std::memset(this->gpr, 0, sizeof(this->gpr));
    this->gpr[10] = this->coreID;

    #ifdef CONFIG_ICache
    this->icache_fence();
    #endif
    this->tlb_fence();
}

uint64_t RVCore::get_uptime() {
    return this->aclint->get_uptime();
}

void RVCore::do_invalid_inst() {
    this->state = ERROR;
    this->haltPC = this->pc;
    WARN("Invalid instruction at pc=" FMT_WORD ", inst=" FMT_WORD32, this->pc, this->inst);

    // Illegal instruction trap
    this->trap(TrapCode::ILLEGAL_INST, this->inst);
}

void RVCore::do_invalid_inst(const DecodeInfo &) {
    this->do_invalid_inst();
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
    } 
    
    if (name == "mstatus") {
        return this->get_csr_core(CSRAddr::MSTATUS);
    } else if (name == "misa") {
        return this->get_csr_core(CSRAddr::MISA);
    } else if (name == "medeleg") {
        return this->get_csr_core(CSRAddr::MEDELEG);
    } else if (name == "mideleg") {
        return this->get_csr_core(CSRAddr::MIDELEG);
    } else if (name == "mie") {
        return this->get_csr_core(CSRAddr::MIE);
    } else if (name == "mtvec") {
        return this->get_csr_core(CSRAddr::MTVEC);
    } else if (name == "mcnten") {
        return this->get_csr_core(CSRAddr::MCNTEN);
    }
    
    if (name == "mscratch") {
        return this->get_csr_core(CSRAddr::MSCRATCH);
    } else if (name == "mepc") {
        return this->get_csr_core(CSRAddr::MEPC);
    } else if (name == "mcause") {
        return this->get_csr_core(CSRAddr::MCAUSE);
    } else if (name == "mtval") {
        return this->get_csr_core(CSRAddr::MTVAL);
    } else if (name == "mip") {
        return this->get_csr_core(CSRAddr::MIP);
    } else if (name == "mtinst") {
        return this->get_csr_core(CSRAddr::MTINST);
    } else if (name == "mtval2") {
        return this->get_csr_core(CSRAddr::MTVAL2);
    }

    if (name == "sstatus") {
        return this->get_csr_core(CSRAddr::SSTATUS);
    } else if (name == "sie") {
        return this->get_csr_core(CSRAddr::SIE);
    } else if (name == "stvec") {
        return this->get_csr_core(CSRAddr::STVEC);
    } else if (name == "scnten") {
        return this->get_csr_core(CSRAddr::SCNTEN);
    }
    
    if (name == "sscratch") {
        return this->get_csr_core(CSRAddr::SSCRATCH);
    } else if (name == "sepc") {
        return this->get_csr_core(CSRAddr::SEPC);
    } else if (name == "scause") {
        return this->get_csr_core(CSRAddr::SCAUSE);
    } else if (name == "stval") {
        return this->get_csr_core(CSRAddr::STVAL);
    } else if (name == "sip") {
        return this->get_csr_core(CSRAddr::SIP);
    } else if (name == "stimecmp") {
        return this->get_csr_core(CSRAddr::STIMECMP);
    }
    
    if (name == "satp") {
        return this->get_csr_core(CSRAddr::SATP);
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
    } else if (name == "mstatus") {
        this->set_csr_core(CSRAddr::MSTATUS, value);
        return true;
    } else if (name == "misa") {
        this->set_csr_core(CSRAddr::MISA, value);
        return true;
    } else if (name == "medeleg") {
        this->set_csr_core(CSRAddr::MEDELEG, value);
        return true;
    } else if (name == "mideleg") {
        this->set_csr_core(CSRAddr::MIDELEG, value);
        return true;
    } else if (name == "mie") {
        this->set_csr_core(CSRAddr::MIE, value);
        return true;
    } else if (name == "mtvec") {
        this->set_csr_core(CSRAddr::MTVEC, value);
        return true;
    } else if (name == "mcnten") {
        this->set_csr_core(CSRAddr::MCNTEN, value);
        return true;
    } else if (name == "mscratch") {
        this->set_csr_core(CSRAddr::MSCRATCH, value);
        return true;
    } else if (name == "mepc") {
        this->set_csr_core(CSRAddr::MEPC, value);
        return true;
    } else if (name == "mcause") {
        this->set_csr_core(CSRAddr::MCAUSE, value);
        return true;
    } else if (name == "mtval") {
        this->set_csr_core(CSRAddr::MTVAL, value);
        return true;
    } else if (name == "mip") {
        this->set_csr_core(CSRAddr::MIP, value);
        return true;
    } else if (name == "mtinst") {
        this->set_csr_core(CSRAddr::MTINST, value);
        return true;
    } else if (name == "mtval2") {
        this->set_csr_core(CSRAddr::MTVAL2, value);
        return true;
    } 
    
    if (name == "sstatus") {
        this->set_csr_core(CSRAddr::SSTATUS, value);
        return true;
    } else if (name == "sie") {
        this->set_csr_core(CSRAddr::SIE, value);
        return true;
    } else if (name == "stvec") {
        this->set_csr_core(CSRAddr::STVEC, value);
        return true;
    } else if (name == "scnten") {
        this->set_csr_core(CSRAddr::SCNTEN, value);
        return true;
    } else if (name == "sscratch") {
        this->set_csr_core(CSRAddr::SSCRATCH, value);
        return true;
    } else if (name == "sepc") {
        this->set_csr_core(CSRAddr::SEPC, value);
        return true;
    } else if (name == "scause") {
        this->set_csr_core(CSRAddr::SCAUSE, value);
        return true;
    } else if (name == "stval") {
        this->set_csr_core(CSRAddr::STVAL, value);
        return true;
    } else if (name == "sip") {
        this->set_csr_core(CSRAddr::SIP, value);
        return true;
    } else if (name == "stimecmp") {
        this->set_csr_core(CSRAddr::STIMECMP, value);
        return true;
    } else if (name == "satp") {
        this->set_csr_core(CSRAddr::SATP, value);
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
