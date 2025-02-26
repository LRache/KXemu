#include "cpu/riscv/core.h"
#include "cpu/riscv/def.h"
#include "macro.h"
#include "debug.h"
#include "word.h"

#include <cstdint>

using namespace kxemu::cpu;

RVCore::do_inst_t RVCore::decode() {
    uint32_t inst = this->inst;
    #include "./autogen/base-decoder.h"
}

RVCore::do_inst_t RVCore::decode_c() {
    uint32_t inst = this->inst & 0xffff;
    #include "./autogen/compressed-decoder.h"
}

bool RVCore::decode_and_exec() {
    do_inst_t do_inst = this->decode();
    if (likely(do_inst != nullptr)) {
        (this->*do_inst)();
        this->add_to_icache(this->pc, this->inst, do_inst, 4);
        return true;
    } else {
        return false;
    } 
}

bool RVCore::decode_and_exec_c() {
    do_inst_t do_inst = this->decode_c();
    if (likely(do_inst != nullptr)) {
        (this->*do_inst)();
        this->add_to_icache(this->pc, this->inst & 0xffff, do_inst, 2);
        return true;
    } else {
        return false;
    }
}

void RVCore::add_to_icache(word_t pc, uint32_t inst, do_inst_t do_inst, uint8_t instLen) {
    word_t set = ICACHE_SET(pc);
    SELF_PROTECT(set < sizeof(this->icache) / sizeof(this->icache[0]), "icache set index out of range");
    this->icache[set].valid = true;
    this->icache[set].inst = this->inst;
    this->icache[set].tag = ICACHE_TAG(pc);
    this->icache[set].do_inst = do_inst;
    this->icache[set].instLen = instLen;
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

bool RVCore::icache_hit_and_exec(word_t pc) {
    word_t set = ICACHE_SET(pc);
    if (this->icache[set].valid && this->icache[set].tag == ICACHE_TAG(pc)) {
        
        this->npc = this->pc + this->icache[set].instLen;

        this->inst = this->icache[set].inst;
        (this->*this->icache[set].do_inst)();
        
        return true;
    
    } else {
        return false;
    }
}

void RVCore::execute() {    
    if (unlikely(this->pc & 0x1)) {
        // Instruction address misaligned
        this->trap(TRAP_INST_ADDR_MISALIGNED);
        return;
    }

    if (this->icache_hit_and_exec(this->pc)) {
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
