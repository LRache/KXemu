#ifndef __RV32_H__
#define __RV32_H__

#define INIT_PC 0x80000000

#define MAX_INST_LEN 4

enum RVFlag {
    RV64  = 1,
    RV128 = 2,
    E     = 4,
    C     = 8,
};

#endif
