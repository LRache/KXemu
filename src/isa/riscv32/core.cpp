#include "isa/riscv32/core.h"
#include "isa/riscv32/privileged-def.h"
#include "isa/word.h"
#include "log.h"
#include "macro.h"
#include <cstdint>

void RV32Core::init(Memory *memory, int flags) {
    this->memory = memory;
    this->flags = flags;
    this->state = IDLE;

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
    auto start = std::chrono::high_resolution_clock::now();

    this->inst = this->memory->read(this->pc, 4);
    
    // try decode and execute the full instruction
    this->npc = this->pc + 4;
    bool valid = this->decoder.decode_and_exec(this->inst);
    // DEBUG("Exec to pc=" FMT_WORD, this->pc);
    if (unlikely(!valid)) {
        // try decode and execute the compressed instruction
        // INFO("Try to decode compressed instruction");
        this->npc = this->pc + 2;
        valid = this->cdecoder.decode_and_exec(this->inst);
        if (unlikely(!valid)) {
            this->do_invalid_inst();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    this->uptime += end - start;
}

word_t RV32Core::memory_read(word_t addr, int len) {
    // mtime memory mapped register
    if (unlikely(addr - MTIME_ADDR < 8)) {
        word_t offset = addr - MTIME_ADDR;
        if (offset == 0) {
            mtime = std::chrono::duration_cast<std::chrono::nanoseconds>(this->uptime).count();
            return mtime & 0xffffffff;
        } else if (offset == 4) {
            return mtime >> 32;
        }
        return 0;
    }

    return this->memory->read(addr, len);
}

int RV32Core::memory_write(word_t addr, word_t data, int len) {
    this->memory->write(addr, data, len);
    return 0;
}

void RV32Core::do_invalid_inst() {
    this->state = ERROR;
    this->haltPC = this->pc;
    WARN("Invalid instruction at pc=" FMT_WORD ", inst=" FMT_WORD, this->pc, this->inst);
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
