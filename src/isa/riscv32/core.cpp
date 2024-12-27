#include "isa/riscv32/core.h"
#include "isa/riscv32/csr-def.h"
#include "isa/word.h"
#include "log.h"
#include "macro.h"

#include <cstdint>

void RV32Core::init(Bus *bus, int flags) {
    this->bus = bus;
    this->flags = flags;
    this->state = IDLE;

    this->privMode = PrivMode::MACHINE;
    this->timerIntrruptNotTriggered = false;

    this->init_decoder();
    this->init_csr();
}

void RV32Core::reset(word_t entry) {
    this->pc = entry;
    this->state = RUNNING;
}

void RV32Core::step() {
    if (likely(this->state == RUNNING)) {
        this->execute();
        this->pc = this->npc;
    } else {
        WARN("Core is not running, nothing to do.");
    }
}

void RV32Core::execute() {
    struct timespec startTime, endTime;

    clock_gettime(CLOCK_MONOTONIC, &startTime);

    if (unlikely(this->timerIntrruptNotTriggered && this->uptime >= this->uptimecmp)) {
        this->interrupt(7);
        this->timerIntrruptNotTriggered = false;
        return;
    }
    if (unlikely(this->scan_interrupt())) {
        return;
    }
    
    if (unlikely(this->pc & 0x1)) {
        this->trap(0); // instruction address misaligned
        return;
    }
    
    if (!this->fetch_inst()) {
        return;
    }
    
    bool valid;
    if (likely((this->inst & 0x3) == 0x3)) {
        this->npc = this->pc + 4;
        valid = this->decoder.decode_and_exec(this->inst);
    } else {
        this->npc = this->pc + 2;
        valid = this->cdecoder.decode_and_exec(this->inst);
    }

    if (unlikely(!valid)) {
        this->do_invalid_inst();
    }

    clock_gettime(CLOCK_MONOTONIC, &endTime);
    uint64_t start = startTime.tv_sec * 1e9 + startTime.tv_nsec;
    uint64_t end = endTime.tv_sec * 1e9 + endTime.tv_nsec;
    this->uptime += end - start;
}

bool RV32Core::fetch_inst() {
    if (unlikely(!(this->privMode == PrivMode::MACHINE || this->check_pmp(this->pc, 4, MemType::FETCH)))) {
        this->trap(EXCP_LOAD_ACCESS_FAULT);
        return false;
    }
    this->inst = this->memory_load(this->pc, 4);
    return true;
}

word_t RV32Core::memory_load(word_t addr, int len) {
    if (unlikely(!(this->privMode == PrivMode::MACHINE || this->check_pmp(addr, len, MemType::LOAD)))) {
        this->trap(EXCP_LOAD_ACCESS_FAULT);
        return -1;
    }
    
    // mtime memory mapped register
    if (unlikely(addr - MTIME_ADDR < 8)) {
        word_t offset = addr - MTIME_ADDR;
        if (offset == 0) {
            // mtime = std::chrono::duration_cast<std::chrono::nanoseconds>(this->uptime).count() / 100;
            mtime = uptime / 100;
            return mtime & 0xffffffff;
        } else if (offset == 4) {
            return mtime >> 32;
        }
        return 0;
    } 
    // mtimecmp memory mapped register
    else if (unlikely(addr - MTIMECMP_ADDR < 8)) {
        word_t offset = addr - MTIMECMP_ADDR;
        if (offset == 0) {
            return mtimecmp & 0xffffffff;
        } else if (offset == 4) {
            return mtimecmp >> 32;
        }
        return -1;
    }

    bool valid;
    return this->bus->read(addr, len, valid);
}

bool RV32Core::memory_store(word_t addr, word_t data, int len) {
    // check PMP
    if (unlikely(!(this->privMode == PrivMode::MACHINE || this->check_pmp(addr, len, MemType::STROE)))) {
        this->trap(EXCP_STORE_ACCESS_FAULT);
        return false;
    }

    // mtimecmp memory mapped register
    if (unlikely(addr - MTIMECMP_ADDR < 8)) {
        word_t offset = addr - MTIMECMP_ADDR;
        if (offset == 0) {
            this->mtimecmp &= ~0xffffffffUL;
            this->mtimecmp |= data;
        } else if (offset == 4) {
            this->mtimecmp &= 0xffffffffUL;
            this->mtimecmp |= (uint64_t)data << 32;
            // this->uptimecmp = std::chrono::duration<uint64_t, std::nano>(this->mtimecmp * 100);
            this->uptimecmp = mtimecmp * 100;
            this->timerIntrruptNotTriggered = true;
        }
        return false;
    }
    
    this->bus->write(addr, data, len);
    
    return true;
}

void RV32Core::do_invalid_inst() {
    this->state = ERROR;
    this->haltPC = this->pc;
    WARN("Invalid instruction at pc=" FMT_WORD ", inst=" FMT_WORD, this->pc, this->inst);

    // illegal instruction trap
    this->trap(2, this->inst);
}

void RV32Core::set_gpr(int index, word_t value) {
    if (likely(index != 0)) {
        this->gpr[index] = value;
        DEBUG("set gpr[%d] = " FMT_WORD_32, index, value);
    }
}

bool RV32Core::is_error() {
    return this->state == ERROR;
}

bool RV32Core::is_break() {
    return this->state == BREAK;
}

bool RV32Core::is_running() {
    return this->state == RUNNING;
}

word_t RV32Core::get_pc() {
    return this->pc;
}

word_t RV32Core::get_gpr(int idx) {
    if (idx >= 32 || idx < 0) {
        WARN("GPR index=%d out of range, return 0 instead.", idx);
        return 0;
    }
    return gpr[idx];
}

word_t RV32Core::get_halt_pc() {
    return this->haltPC;
}

word_t RV32Core::get_halt_code() {
    return this->haltCode;
}

RV32Core::~RV32Core() {}
