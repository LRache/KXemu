#include "cpu/riscv/cpu.h"
#include "device/bus.h"
#include "log.h"

using namespace kxemu::cpu;
using namespace kxemu::device;

void RVCPU::init(Bus *memory, int flags, unsigned int coreCount) {
    if (coreCount != 1) {
        PANIC("RV32CPU does not support multi-core");
    }
    this->aclint.init(coreCount);
    this->cores = new RVCore[coreCount];
    for (unsigned int i = 0; i < coreCount; i++) {
        this->cores[i].init(i, memory, flags, &aclint, &taskTimer);
    }
    this->coreCount = coreCount;
}

void RVCPU::reset(word_t pc) {
    for (unsigned int i = 0; i < coreCount; i++) {
        cores[i].reset(pc);
    }
    aclint.reset();
}

void RVCPU::step() {
    for (unsigned int i = 0; i < coreCount; i++) {
        cores[i].step();
    }
}

bool RVCPU::is_running() {
    return cores[0].is_running();
}

unsigned int RVCPU::core_count() {
    return coreCount;
}

RVCore *RVCPU::get_core(unsigned int coreID) {
    if (coreID >= coreCount) {
        PANIC("Core ID %d is out of range", coreID);
    }
    return &cores[coreID];
}

RVCPU::~RVCPU() {
    delete []cores;
}
