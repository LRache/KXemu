#ifndef __KXEMU_ISA_ISA_H__
#define __KXEMU_ISA_ISA_H__

#include "config/config.h"
#include "cpu/cpu.h"

#include <string>
#include <cstdint>

namespace kxemu::isa {
    #if defined(KXEMU_ISA32)
        using word_t = uint32_t;
    #elif defined(KXEMU_ISA64)
        using word_t = uint64_t;
    #endif
    
    void init();

    // ISA information
    const char *get_isa_name();
    int get_elf_expected_machine();
    const char *get_gdb_target_desc();

    cpu::CPU<word_t> *new_cpu();

    // Register
    unsigned int get_gpr_count();
    const char *get_gpr_name(int idx);

    // Disassemble
    unsigned int get_max_inst_len();
    std::string disassemble(const uint8_t *data, const uint64_t dataSize, const word_t pc, uint64_t &instLen);
} // namespace kxemu::isa

#endif
