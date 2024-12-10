#include "isa/riscv32/cpu.h"
#include "log.h"

void RV32CPU::init(Memory *memory, int flags, int coreCount) {
    if (coreCount != 1) {
        PANIC("RV32CPU does not support multi-core");
    }
    this->core = new RV32Core();
    this->core->init(memory, flags);
    this->coreCount = coreCount;
}

void RV32CPU::reset() {
    core->reset();
}

void RV32CPU::step() {
    core->step();
}

bool RV32CPU::has_break() {
    return core->is_break() || core->is_error();
}

int RV32CPU::core_count() {
    return coreCount;
}

Core *RV32CPU::get_core(int coreID) {
    if (coreID != 0) {
        PANIC("RV32CPU does not support multi-core");
    }
    return core;
}

RV32CPU::~RV32CPU() {
    delete core;
}
