#include "cpu/riscv/core.h"
#include "cpu/riscv/aclint.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"
#include "device/bus.h"
#include "log.h"
#include "macro.h"

#include <unordered_set>
#include <cstdint>
#include <cstring>

using namespace kxemu::cpu;
using kxemu::utils::TaskTimer;

RVCore::RVCore() {    
    this->mstatus = this->csr.get_csr_ptr(CSR_MSTATUS);
    this->medeleg = this->csr.get_csr_ptr_readonly(CSR_MEDELEG);
    this->mideleg = this->csr.get_csr_ptr_readonly(CSR_MIDELEG);
    this->mie     = this->csr.get_csr_ptr_readonly(CSR_MIE);
    this->mip     = this->csr.get_csr_ptr(CSR_MIP);

    // RV32 only
#ifdef KXEMU_ISA32
    this->medelegh= this->csr.get_csr_ptr_readonly(CSR_MEDELEGH);
#endif

    this->satp    = this->csr.get_csr_ptr_readonly(CSR_SATP);
}

void RVCore::init(unsigned int coreID, device::Bus *bus, int flags, device::AClint *alint, TaskTimer *timer) {
    this->coreID = coreID;
    this->bus = bus;
    this->flags = flags;
    this->aclint = alint;
    this->state = IDLE;

    this->init_csr();
}

void RVCore::reset(word_t entry) {
    this->pc = entry;
    this->state = RUNNING;
    this->privMode = PrivMode::MACHINE;
    
    this->csr.reset();

    std::memset(this->gpr, 0, sizeof(this->gpr));
}

void RVCore::step() {
    if (unlikely(this->state == BREAKPOINT)) {
        this->state = RUNNING;
    }
    if (likely(this->state == RUNNING)) {
        // Interrupt
        this->bus->update();
        this->scan_interrupt();
        
        this->execute();
        this->pc = this->npc;
    } else {
        WARN("Core is not running, nothing to do.");
    }
}

void RVCore::run(const word_t *breakpoints_, unsigned int n) {
    std::unordered_set<word_t> breakpoints;
    for (unsigned int i = 0; i < n; i++) {
        breakpoints.insert(breakpoints_[i]);
    }

    unsigned int i = 0;
    this->state = RUNNING;
    while (this->state == RUNNING) {
        if (breakpoints.find(this->pc) != breakpoints.end()) {
            this->haltCode = 0;
            this->haltPC = this->pc;
            this->state = BREAKPOINT;
            break;
        }
        
        // Interrupt
        if (unlikely(i & 0x2000)) {
            this->bus->update();
            this->scan_interrupt();
            i = 0;
        }
        
        this->execute();
        this->pc = this->npc;
        i++;
    }
}

void RVCore::execute() {    
    if (unlikely(this->pc & 0x1)) {
        // Instruction address misaligned
        this->trap(TRAP_INST_ADDR_MISALIGNED);
        return;
    }
    
    if (!this->memory_fetch()) {
        return;
    }
    
    bool valid;
    if (likely((this->inst & 0x3) == 0x3)) {
        this->npc = this->pc + 4;
        valid = this->decode_and_exec();
    } else {
        this->npc = this->pc + 2;
        valid = this->decode_and_exec_c();
    }

    if (unlikely(!valid)) {
        this->do_invalid_inst();
    }
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

word_t RVCore::get_gpr(unsigned int idx) {
    if (idx >= 32 || idx < 0) {
        WARN("GPR index=%d out of range, return 0 instead.", idx);
        return 0;
    }
    return gpr[idx];
}

void RVCore::set_gpr(unsigned int index, word_t value) {
    this->gpr[index] = value;
    this->gpr[0] = 0;
}

word_t RVCore::get_register(const std::string &name, bool &success) {
    success = true;
    if (name == "pc") {
        return this->pc;
    } else if (name == "mstatus") {
        return *this->mstatus;
    } else if (name == "mie") {
        return *this->mie;
    } else if (name == "mip") {
        return *this->mip;
    } else if (name == "medeleg") {
        return *this->medeleg;
    } else {
        success = false;
        return 0;
    }
}

word_t RVCore::get_halt_pc() {
    return this->haltPC;
}

word_t RVCore::get_halt_code() {
    return this->haltCode;
}

RVCore::~RVCore() {}
