#ifndef __KXEMU_ISA_RISCV32_ISA_H__
#define __KXEMU_ISA_RISCV32_ISA_H__

enum RVFlag {
    E = 1,
    C = 2,
    M = 4,
    Zicsr = 8,
    Priv  = 16
};

#endif
