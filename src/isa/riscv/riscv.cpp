#include "cpu/cpu.h"
#include "isa/isa.h"
#include "cpu/riscv/cpu.h"

#include <elf.h>

using namespace kxemu;

static const char* gprNames[] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

unsigned int isa::get_gpr_count() {
    return 32;
}

const char *kxemu::isa::get_gpr_name(int idx) {
    return gprNames[idx];
}

const char *kxemu::isa::get_isa_name() {
    #ifdef KXEMU_ISA64
        return "riscv64";
    #else
        return "riscv32";
    #endif
}

int kxemu::isa::get_elf_expected_machine() {
    return EM_RISCV;
}

extern void init_disasm();

void kxemu::isa::init() {
    init_disasm();
}

cpu::CPU<isa::word_t> *kxemu::isa::new_cpu() {
    return new cpu::RVCPU();
}
