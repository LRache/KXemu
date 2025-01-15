#ifndef __KXEMU_ISA_ISA_H__
#define __KXEMU_ISA_ISA_H__

#include "config/config.h"

#ifdef ISA
    #if ISA == riscv32
        #include "cpu/riscv32/cpu.h"
        #include "isa/riscv/isa.h"
        #define ISA_CPU RV32CPU
        #define ISA_NAME "riscv32"
    #else
        #error "Unsupport ISA"
    #endif
#else
    #error "ISA not defined"
#endif

namespace kxemu::isa {
    const char *get_gpr_name(int idx);
} // namespace kxemu::isa

#endif
