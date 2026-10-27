#include "cpu/cpu.hpp"
#include "isa/isa.hpp"
#include "cpu/riscv/cpu.hpp"
#include "cpu/riscv/platform.hpp"

#include "elf.h"

#include <memory>

using namespace kxemu;

unsigned int isa::get_gpr_count() {
    return 32;
}

const char *kxemu::isa::get_gpr_name(int idx) {
    static const char* gprNames[] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };
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

const char *kxemu::isa::get_gdb_target_desc() {
    #ifdef KXEMU_ISA64
        return "riscv:rv64";
    #else
        return "riscv:rv32";
    #endif
}

extern void init_disasm();

void kxemu::isa::init() {
    init_disasm();
}

cpu::CPU<isa::word_t> *kxemu::isa::new_cpu() {
    return new cpu::RVCPU();
}

#ifndef CONFIG_DIFFTEST
static std::unique_ptr<cpu::RVPlatform> platform;
#endif

// Difftest builds keep bare harts: the framework drives time and interrupts.
void kxemu::isa::init_platform(cpu::CPU<isa::word_t> *cpu, device::Bus *bus) {
#ifndef CONFIG_DIFFTEST
    platform = std::make_unique<cpu::RVPlatform>();
    platform->attach(static_cast<cpu::RVCPU *>(cpu), bus);
#endif
}

void kxemu::isa::reset_platform() {
#ifndef CONFIG_DIFFTEST
    if (platform != nullptr) platform->reset();
#endif
}

void kxemu::isa::deinit_platform() {
#ifndef CONFIG_DIFFTEST
    platform.reset();
#endif
}
