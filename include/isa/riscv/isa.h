#ifndef __KXEMU_ISA_RISCV32_ISA_H__
#define __KXEMU_ISA_RISCV32_ISA_H__

enum RVFlag {
    E = 1 << 0,
    C = 1 << 1,
    M = 1 << 2,
    A = 1 << 3,
    Zicsr = 1 << 4,
    Priv  = 1 << 5,
};

#endif
