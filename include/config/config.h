#ifndef __KXEMU_CONFIG_CONFIG_H__
#define __KXEMU_CONFIG_CONFIG_H__

#include "autoconf.h"
#include "macro.h"

#if defined (CONFIG_ISA_riscv32) && CONFIG_ISA_riscv32 == 1
    #define ISA riscv32
    #define ISA32
#elif defined (CONFIG_ISA_riscv64) && CONFIG_ISA_riscv64 == 1
    #define ISA riscv64
    #define ISA64
#else
    #error "Unsupport ISA"
#endif

#define ISA_NAME MACRO_TO_STRING(ISA)

#endif
