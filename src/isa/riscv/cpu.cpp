#include "isa/riscv/cpu.h"
#include "log.h"

void RV32CPU::init(Memory *memory, int flags, int coreCount) {
    if (coreCount != 1) {
        PANIC("RV32CPU does not support multi-core");
    }
    core = new RV32Core();
    core->init(memory, flags);
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

Core *RV32CPU::getCore(int coreID) {
    if (coreID != 0) {
        PANIC("RV32CPU does not support multi-core");
    }
    return core;
}

RV32CPU::~RV32CPU() {
    delete core;
}
