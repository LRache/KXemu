#ifndef __ISA_H__
#define __ISA_H__

enum ISA {
    RISCV32,
    RISCV64,
    X86,
    ARM,
    MIPS,
    LoongArch,
};

static const char *ISA_NAME[] = {
    "RISCV32",
    "RISCV64",
    "X86",
    "ARM",
    "MIPS",
    "LoongArch",
};

static inline const char *isa_name(ISA isa) {
    return ISA_NAME[isa];
}

#endif
