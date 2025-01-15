#include "cpu/riscv32/cpu.h"
#include "isa/isa.h"

using namespace kxemu;

cpu::CPU<isa::word_t> *isa::new_cpu() {
    return new cpu::RV32CPU;
}
