#include "config/config.h"
#include "debug.h"

#ifdef CONFIG_DEBUG
    #define RD  unsigned int rd  = decodeInfo.rd ; SELF_PROTECT(decodeInfo.rd_set , "decodeInfo.rd_set is false.");
    #define RS1 unsigned int rs1 = decodeInfo.rs1; SELF_PROTECT(decodeInfo.rs1_set, "decodeInfo.rs1_set is false.");
    #define RS2 unsigned int rs2 = decodeInfo.rs2; SELF_PROTECT(decodeInfo.rs2_set, "decodeInfo.rs2_set is false.");
    #define CSR unsigned int csr = decodeInfo.csr; SELF_PROTECT(decodeInfo.csr_set, "decodeInfo.csr_set is false.");
    #define IMM word_t       imm = decodeInfo.imm; SELF_PROTECT(decodeInfo.imm_set, "decodeInfo.imm_set is false.");
#else
    #define RD  unsigned int rd  = decodeInfo.rd ;
    #define RS1 unsigned int rs1 = decodeInfo.rs1;
    #define RS2 unsigned int rs2 = decodeInfo.rs2;
    #define CSR unsigned int csr = decodeInfo.csr;
    #define IMM unsigned int imm = decodeInfo.imm;
#endif
