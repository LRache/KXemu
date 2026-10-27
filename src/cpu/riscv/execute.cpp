#include "cpu/riscv/core.hpp"
#include "cpu/word.hpp"
#include "macro.h"
#include "log.h"

#include <cstring>
#include <unordered_set>

using namespace kxemu::cpu;

RVCore::do_inst_t RVCore::decode_and_exec(DecodeInfo &decodeInfo) {
    #include "./autogen/base-decoder.inc"
}

RVCore::do_inst_t RVCore::decode_and_exec_c(DecodeInfo &decodeInfo) {
    this->inst = this->inst & 0xffff;
    #include "./autogen/compressed-decoder.inc"
}

void RVCore::step() {
    if (this->stop_requested()) {
        if (this->state == RUNNING) this->state = IDLE;
        return;
    }
    if (unlikely(this->state == BREAKPOINT)) {
        this->state = RUNNING;
    }
    if (likely(this->state == RUNNING)) {
        // Interrupt. Devices push their IRQs, so there is nothing to poll.
        // An interrupt is taken before the instruction at pc: the trap returns to pc,
        // and this step ends at the trap vector without retiring an instruction.
        this->npc = this->pc;
        if (this->scan_interrupt()) {
            this->pc = this->npc;
            return;
        }

        this->execute();
        this->pc = this->npc;
    } else {
        WARN("Core is not running, nothing to do.");
    }
}

void RVCore::run_step() {
    this->execute();

    // Interrupt. Producers and the hart itself flag a possibly deliverable interrupt,
    // so it traps before the next instruction: xv6 re-disables right after `intr_on()`.
    // The plain load keeps the common path free of a read-modify-write.
    if (unlikely(this->interruptPending.load(std::memory_order_relaxed))) {
        if (this->interruptPending.exchange(false, std::memory_order_acquire)) {
            this->scan_interrupt();
        }
    }

    this->pc = this->npc;
}

void RVCore::run(const word_t *breakpoints_, unsigned int n) {
    std::unordered_set<word_t> breakpoints;
    for (unsigned int i = 0; i < n; i++) {
        breakpoints.insert(breakpoints_[i]);
    }

    this->state = RUNNING;
    if (n == 0) {
        while (this->state == RUNNING && !this->stop_requested()) {
            this->run_step();
        }
    } else {
        if (this->state == RUNNING && !this->stop_requested()) {
            this->run_step();
        }
        while (this->state == RUNNING && !this->stop_requested()) {
            if (breakpoints.find(this->pc) != breakpoints.end()) {
                this->haltCode = 0;
                this->haltPC = this->pc;
                this->state = BREAKPOINT;
                break;
            }

            this->run_step();
        }
    }
    if (this->state == RUNNING && this->stop_requested()) {
        this->state = IDLE;
    }
}

void RVCore::execute() {    
    try {
        if (unlikely(this->pc & 1)) {
            throw TrapException(TrapCode::INST_ADDR_MISALIGNED, this->pc);
        }
        
        if (likely(this->icache_decode_and_exec())) {
            return;
        }
        
        this->memory_fetch();

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
    } catch (const TrapException &e) {
        // Handle trap exception
        this->enter_trap(e.code(), e.value());
    }
}
