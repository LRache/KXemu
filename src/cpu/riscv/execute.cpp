#include "cpu/riscv/core.h"
#include "cpu/word.h"
#include "macro.h"
#include "log.h"

#include <cstdint>
#include <cstring>

using namespace kxemu::cpu;

RVCore::do_inst_t RVCore::decode_and_exec(DecodeInfo &decodeInfo) {
    uint32_t inst = this->inst;
    #include "./autogen/base-decoder.inc"
}

RVCore::do_inst_t RVCore::decode_and_exec_c(DecodeInfo &decodeInfo) {
    uint32_t inst = this->inst & 0xffff;
    #include "./autogen/compressed-decoder.inc"
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

void RVCore::set_device_mtx(std::mutex *mtx) {
    this->deviceMtx = mtx;
}

void RVCore::run_step(unsigned int &counter) {
    constexpr unsigned int interruptFreq = 0x1000;
    
    this->execute();

    // Interrupt
    if (unlikely(counter & interruptFreq)) {
        this->update_device();
        this->scan_interrupt();
        counter = 0;
    }
            
    this->pc = this->npc;
    counter++;
}

void RVCore::run(const word_t *breakpoints_, unsigned int n) {
    std::unordered_set<word_t> breakpoints;
    for (unsigned int i = 0; i < n; i++) {
        breakpoints.insert(breakpoints_[i]);
    }

    unsigned int i = 0;
    this->state = RUNNING;
    if (n == 0) {
        while (this->state == RUNNING) {            
            this->run_step(i);
        }
    } else {
        if (this->state == RUNNING) {
            this->run_step(i);
        }
        while (this->state == RUNNING) {
            if (breakpoints.find(this->pc) != breakpoints.end()) {
                this->haltCode = 0;
                this->haltPC = this->pc;
                this->state = BREAKPOINT;
                break;
            }

            this->run_step(i);
        }
    }
}

void RVCore::update_device() {
    if (unlikely(this->deviceMtx != nullptr)) {
        if (unlikely(this->deviceMtx->try_lock())) {
            this->bus->update();
            this->plic->scan_and_set_interrupt(this->coreID, this->privMode);
            this->deviceMtx->unlock();
        }
    } else {
        this->bus->update();
        this->plic->scan_and_set_interrupt(this->coreID, this->privMode);
    }
}

void RVCore::execute() {
    if (unlikely(this->pc & 0x1)) {
        // Instruction address misaligned
        this->trap(TrapCode::INST_ADDR_MISALIGNED);
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
    DecodeInfo decodeInfo;
    if (unlikely((this->inst & 0x3) == 0x3)) {
        this->npc = this->pc + 4;
        instLen = 4;
        do_inst = this->decode_and_exec(decodeInfo);
    } else {
        this->npc = this->pc + 2;
        instLen = 2;
        do_inst = this->decode_and_exec_c(decodeInfo);
    }
    
    if (unlikely(do_inst == nullptr)) {
        this->do_invalid_inst();
    } else {
        this->icache_push(do_inst, instLen, decodeInfo);
    }
}
