#ifndef __KXEMU_CONFIG_CONFIG_H__
#define __KXEMU_CONFIG_CONFIG_H__

#include "autoconf.h"

#if defined (CONFIG_ISA_riscv32) && CONFIG_ISA_riscv32 == 1
    #define ISA riscv32
#elif defined (CONFIG_ISA_riscv64) && CONFIG_ISA_riscv64 == 1
    #define ISA riscv64
    #define ISA_64
#endif

#endif
