#include "cpu/cpu.h"
#include "isa/isa.h"
#include "config/config.h"
#include "cpu/loongarch/cpu.h"

#include <elf.h>

using namespace kxemu;

void isa::init() {}

const char *isa::get_isa_name() {
#ifdef KXEMU_ISA32
    return "loongarch32";
#else
    return "loongarch64";
#endif
}

int isa::get_elf_expected_machine() {
    return EM_LOONGARCH;
}

cpu::CPU<isa::word_t> *isa::new_cpu() {
    return new cpu::LACPU();
}

unsigned int isa::get_gpr_count() {
    return 32;
}

const char *isa::get_gpr_name(int idx) {
    static const char* gprNames[] = {
        "zero", "ra", "tp", "sp",
        "a0", "a1", "a2", "a3",
        "a4", "a5", "a6", "a7",
        "t0", "t1", "t2", "t3",
        "t4", "t5", "t6", "t7",
        "t8", "u0", "fp", "s0",
        "s1", "s2", "s3", "s4",
        "s5", "s6", "s7", "s8",
    };
    return gprNames[idx];
}

unsigned int isa::get_max_inst_len() {
    return 4;
}

std::string isa::disassemble(const uint8_t *data, const uint64_t dataSize, const word_t pc, uint64_t &instLen) {
    instLen = 4;
    return "";
}
