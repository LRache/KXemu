#include "cpu/riscv/core.h"
#include "cpu/riscv/def.h"
#include "cpu/word.h"
#include "macro.h"
#include "log.h"

#include <cstdint>
#include <cstring>

#include <immintrin.h>

using namespace kxemu::cpu;

RVCore::do_inst_t RVCore::decode_and_exec() {
    uint32_t inst = this->inst;
    #include "./autogen/base-decoder.h"
}

RVCore::do_inst_t RVCore::decode_and_exec_c() {
    uint32_t inst = this->inst & 0xffff;
    #include "./autogen/compressed-decoder.h"
}

void RVCore::step() {
    if (unlikely(this->state == BREAKPOINT)) {
        this->state = RUNNING;
    }
    if (likely(this->state == RUNNING)) {
        // Interrupt
        this->bus->update();
        this->plic->scan_and_set_interrupt(this->coreID, this->privMode);
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

    constexpr unsigned int interruptFreq = 0x1000;
    unsigned int i = 0;
    this->state = RUNNING;
    if (n == 0) {
        while (this->state == RUNNING) {            
            
            this->execute();

            // Interrupt
            if (unlikely(i & interruptFreq)) {
                this->update_device();
                this->scan_interrupt();
                i = 0;
            }
            
            this->pc = this->npc;
            
            i++;
        }
    } else {
        while (this->state == RUNNING) {
            this->execute();

            // Interrupt
            if (unlikely(i & interruptFreq)) {
                this->update_device();
                this->scan_interrupt();
                i = 0;
            }
            
            this->pc = this->npc;

            i++;

            if (breakpoints.find(this->pc) != breakpoints.end()) {
                this->haltCode = 0;
                this->haltPC = this->pc;
                this->state = BREAKPOINT;
                break;
            }
        }
    }
}

void RVCore::update_device() {
    if (this->bus->get_lock()->try_lock()) {
        this->bus->update();
        this->bus->get_lock()->unlock();
    }
    
    this->plic->get_lock()->lock();
    this->plic->scan_and_set_interrupt(this->coreID, this->privMode);
    this->plic->get_lock()->unlock();
}

void RVCore::execute() {
    if (unlikely(this->pc & 0x1)) {
        // Instruction address misaligned
        this->trap(TRAP_INST_ADDR_MISALIGNED);
        this->pc = this->npc;
        return;
    }

    #ifdef CONFIG_ICache
    if (this->icache_decode_and_exec()) {
        return;
    }
    #endif
    
    if (!this->memory_fetch()) {
        this->pc = this->npc;
        return;
    }
    
    #ifdef CONFIG_DEBUG_DECODER
    std::memset(&this->gDecodeInfo, 0xac, sizeof(this->gDecodeInfo));
    this->gDecodeInfo.rd_set  = false;
    this->gDecodeInfo.rs1_set = false;
    this->gDecodeInfo.rs2_set = false;
    this->gDecodeInfo.csr_set = false;
    this->gDecodeInfo.imm_set = false;
    #endif
    
    unsigned int instLen;
    do_inst_t do_inst;
    if (unlikely((this->inst & 0x3) == 0x3)) {
        this->npc = this->pc + 4;
        instLen = 4;
        do_inst = this->decode_and_exec();
    } 
    else {
        this->npc = this->pc + 2;
        instLen = 2;
        do_inst = this->decode_and_exec_c();
    }
    
    if (unlikely(do_inst == nullptr)) {
        this->do_invalid_inst();
    } else {
        this->icache_push(do_inst, instLen);
    }
}
