#ifndef __ISA_H__
#define __ISA_H__

#ifdef ISA
    #if ISA == RISCV32
        #include "isa/riscv32/cpu.h"
        #define ISA_CPU RV32CPU
        #define ISA_NAME "riscv32"
    #else
        #error "Unsupport ISA"
    #endif
#else
    #error "ISA not defined"
#endif

#endif
