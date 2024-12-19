#ifndef __ISA_RISCV32_ISA_H__
#define __ISA_RISCV32_ISA_H__

#include <string>

#define INIT_PC 0x80000000

#define MAX_INST_LEN 4
#define GPR_COUNT 32

enum RVFlag {
    E = 1,
    C = 2,
    M = 4,
    Zicsr = 8,
    Priv  = 16
};

std::string get_gpr_name(int idx);

#endif
