#include "cpu/loongarch/core.h"
#include "log.h"

#include <cstring>

using namespace kxemu::cpu;

void LACore::step() {
    if (this->stopping.load(std::memory_order_acquire)) {
        if (this->state == RUNNING) this->state = IDLE;
        return;
    }
    this->execute();
}

void LACore::run(const word_t *breakpoints, unsigned int n) {
    this->state = RUNNING;
    while (this->state == RUNNING && !this->stopping.load(std::memory_order_acquire)) {
        this->execute();
    }
    if (this->state == RUNNING && this->stopping.load(std::memory_order_acquire)) {
        this->state = IDLE;
    }
}

void LACore::execute() {
    this->npc = this->pc + 4;
    if (!this->memory_fetch()) {
        this->state = ERROR;
        WARN("Failed to fetch instruction at pc=" FMT_WORD, this->pc);
    }

    if (!this->decode_and_exec()) {
        this->do_illegal_inst();
    }
    this->pc = this->npc;
}

bool LACore::decode_and_exec() {
    #include "./autogen/inst-decoder.inc"
}

void LACore::do_illegal_inst() {
    this->state = ERROR;
    this->haltPC = this->pc;
    WARN("Illegal instruction at pc=" FMT_WORD, this->pc);
}
