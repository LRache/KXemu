#ifndef __KXEMU_CONFIG_CONFIG_H__
#define __KXEMU_CONFIG_CONFIG_H__

#include "config/autoconf.h"
#include "macro.h"

#if defined (CONFIG_ISA_riscv32) && CONFIG_ISA_riscv32 == 1
    #define KXEMU_ISA riscv32
    #define KXEMU_ISA32
    #define KXEMU_ISA_32BIT true
    #define KXEMU_ISA_64BIT false
#elif defined (CONFIG_ISA_riscv64) && CONFIG_ISA_riscv64 == 1
    #define KXEMU_ISA riscv64
    #define KXEMU_ISA64
    #define KXEMU__ISA_32BIT false
    #define KXEMU__ISA_64BIT true
#elif defined (CONFIG_ISA_loongarch32) && CONFIG_ISA_loongarch32 == 1
    #define KXEMU_ISA loongarch32
    #define KXEMU_ISA32
    #define KXEMU_ISA_32BIT true
    #define KXEMU_ISA_64BIT false
#elif defined (CONFIG_ISA_loongarch64) && CONFIG_ISA_loongarch64 == 1
    #define KXEMU_ISA loongarch64
    #define KXEMU_ISA64
    #define KXEMU_ISA_32BIT false
    #define KXEMU_ISA_64BIT true
#else
    #error "Unsupport ISA"
#endif

#define KXEMU_ISA_STR MACRO_TO_STRING(KXEMU_ISA)

#endif
