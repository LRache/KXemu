#include "cpu/riscv64/cpu.h"
#include "cpu/riscv64/core.h"
#include "log.h"

void RV64CPU::init(Bus *memory, int flags, int coreCount) {
    if (coreCount != 1) {
        PANIC("RV64CPU does not support multi-core");
    }
    this->core = new RV64Core();
    this->core->init(memory, flags);
    this->coreCount = coreCount;
}


