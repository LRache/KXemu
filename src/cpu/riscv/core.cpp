#include "cpu/riscv/core.h"
#include "cpu/riscv/aclint.h"
#include "cpu/riscv/def.h"
#include "cpu/riscv/cache-def.h"
#include "cpu/word.h"
#include "debug.h"
#include "device/bus.h"
#include "log.h"
#include "macro.h"
#include "word.h"

#include <cstdint>
#include <cstring>

using namespace kxemu::cpu;

RVCore::RVCore() {    
    this->medeleg = this->csr.get_csr_ptr_readonly(CSR_MEDELEG);
    this->mideleg = this->csr.get_csr_ptr_readonly(CSR_MIDELEG);
    this->mie     = this->csr.get_csr_ptr_readonly(CSR_MIE);
    this->mip     = this->csr.get_csr_ptr(CSR_MIP);

    // RV32 only
#ifdef KXEMU_ISA32
    this->medelegh= this->csr.get_csr_ptr_readonly(CSR_MEDELEGH);
#endif

    this->vaddr_translate_func = &RVCore::vaddr_translate_bare;
}

void RVCore::init(unsigned int coreID, device::Bus *bus, int flags, device::AClint *alint, device::PLIC *plic) {
    this->coreID = coreID;
    this->bus = bus;
    this->flags = flags;
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
    this->trap(TRAP_ILLEGAL_INST, this->inst);
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

word_t RVCore::get_register(const std::string &name, bool &success) {
    success = true;
    
    static const char* gprNames[] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };
    for (unsigned int i = 0; i < 32; i++) {
        if (name == gprNames[i]) {
            return this->gpr[i];
        }
    }

    static const char* gprAlias[] = {
        "x0" , "x1" , "x2" , "x3" , "x4" , "x5" , "x6" , "x7" ,
        "x8" , "x9" , "x10", "x11", "x12", "x13", "x14", "x15",
        "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
        "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"
    };
    for (unsigned int i = 0; i < 32; i++) {
        if (name == gprAlias[i]) {
            return this->gpr[i];
        }
    }

    if (name == "pc") {
        return this->pc;
    } 
    
    if (name == "mstatus") {
        return this->get_csr_core(CSR_MSTATUS);
    } else if (name == "misa") {
        return this->get_csr_core(CSR_MISA);
    } else if (name == "medeleg") {
        return this->get_csr_core(CSR_MEDELEG);
    } else if (name == "mideleg") {
        return this->get_csr_core(CSR_MIDELEG);
    } else if (name == "mie") {
        return this->get_csr_core(CSR_MIE);
    } else if (name == "mtvec") {
        return this->get_csr_core(CSR_MTVEC);
    } else if (name == "mcnten") {
        return this->get_csr_core(CSR_MCNTEN);
    }
    
    if (name == "mscratch") {
        return this->get_csr_core(CSR_MSCRATCH);
    } else if (name == "mepc") {
        return this->get_csr_core(CSR_MEPC);
    } else if (name == "mcause") {
        return this->get_csr_core(CSR_MCAUSE);
    } else if (name == "mtval") {
        return this->get_csr_core(CSR_MTVAL);
    } else if (name == "mip") {
        return this->get_csr_core(CSR_MIP);
    } else if (name == "mtinst") {
        return this->get_csr_core(CSR_MTINST);
    } else if (name == "mtval2") {
        return this->get_csr_core(CSR_MTVAL2);
    }

    if (name == "sstatus") {
        return this->get_csr_core(CSR_SSTATUS);
    } else if (name == "sie") {
        return this->get_csr_core(CSR_SIE);
    } else if (name == "stvec") {
        return this->get_csr_core(CSR_STVEC);
    } else if (name == "scnten") {
        return this->get_csr_core(CSR_SCNTEN);
    }
    
    if (name == "sscratch") {
        return this->get_csr_core(CSR_SSCRATCH);
    } else if (name == "sepc") {
        return this->get_csr_core(CSR_SEPC);
    } else if (name == "scause") {
        return this->get_csr_core(CSR_SCAUSE);
    } else if (name == "stval") {
        return this->get_csr_core(CSR_STVAL);
    } else if (name == "sip") {
        return this->get_csr_core(CSR_SIP);
    } else if (name == "stimecmp") {
        return this->get_csr_core(CSR_STIMECMP);
    }
    
    if (name == "satp") {
        return this->get_csr_core(CSR_SATP);
    }
    
    success = false;
    return 0;
}

word_t RVCore::get_halt_pc() {
    return this->haltPC;
}

word_t RVCore::get_halt_code() {
    return this->haltCode;
}

RVCore::~RVCore() {}
