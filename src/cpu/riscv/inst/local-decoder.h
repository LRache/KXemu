#include "config/config.h"
#include "debug.h"

#define SEXT64(v) ((sword_t)(int32_t)(v))

#ifdef CONFIG_DEBUG
    #define DEST (this->gpr[decodeInfo.rd ])
    #define SRC1 (this->gpr[decodeInfo.rs1])
    #define SRC2 (this->gpr[decodeInfo.rs2])
    #define CSR  ( decodeInfo.csr)
    #define IMM  ( decodeInfo.imm)

    #define  rd_is_x0 (decodeInfo.rd  == 32)
    #define rs1_is_x0 (decodeInfo.rs1 == 0 )
#else
    // #define RD  unsigned int rd  = decodeInfo.rd ;
    // #define RS1 unsigned int rs1 = decodeInfo.rs1;
    // #define RS2 unsigned int rs2 = decodeInfo.rs2;
    // #define CSR unsigned int csr = decodeInfo.csr;
    // #define IMM word_t       imm = decodeInfo.imm;

    // #define DEST (*decodeInfo.rd )
    // #define SRC1 (*decodeInfo.rs1)
    // #define SRC2 (*decodeInfo.rs2)
    #define DEST (this->gpr[decodeInfo.rd ])
    #define SRC1 (this->gpr[decodeInfo.rs1])
    #define SRC2 (this->gpr[decodeInfo.rs2])
    #define CSR  ( decodeInfo.csr)
    #define IMM  ( decodeInfo.imm)

    #define  rd_is_x0 (decodeInfo.rd  == 32)
    #define rs1_is_x0 (decodeInfo.rs1 ==  0)
    // #define  rd_is_x0 (decodeInfo.rd  == &this->gpr[32])
    // #define rs1_is_x0 (decodeInfo.rs1 == &this->gpr[ 0])
#endif
