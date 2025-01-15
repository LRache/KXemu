#ifndef __KXEMU_ISA_ISA_H__
#define __KXEMU_ISA_ISA_H__

#include "config/config.h"

#ifdef ISA
    #if ISA == riscv32
        #include "cpu/riscv32/cpu.h"
        #include "isa/riscv/isa.h"
    #else
        #error "Unsupport ISA"
    #endif
#else
    #error "ISA not defined"
#endif

namespace kxemu::isa {
    #if defined(ISA32)
        using word_t = uint32_t;
    #elif defined(ISA64)
        using word_t = uint64_t;
    #endif
    
    void init();

    cpu::CPU<word_t> *new_cpu();
    const char *get_gpr_name(int idx);

    std::string disassemble(const uint8_t *data, const size_t length, word_t pc, unsigned int &instLen);
} // namespace kxemu::isa

#endif
