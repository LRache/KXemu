#include "config/config.h"
#include "debug.h"

#define SEXT64(v) ((sword_t)(int32_t)(v))

// #define DEST (*decodeInfo.rd )
// #define SRC1 (*decodeInfo.rs1)
// #define SRC2 (*decodeInfo.rs2)
#define DEST (this->gpr[decodeInfo.rd ])
#define SRC1 (this->gpr[decodeInfo.rs1])
#define SRC2 (this->gpr[decodeInfo.rs2])
#define CSR  ( decodeInfo.csr)
#define IMM  ( decodeInfo.imm)

// #define  rd_is_x0 (decodeInfo.rd  == &this->gpr[32])
// #define rs1_is_x0 (decodeInfo.rs1 == &this->gpr[ 0])
#define  rd_is_x0 (decodeInfo.rd  == 32)
#define rs1_is_x0 (decodeInfo.rs1 ==  0)

#ifdef CONFIG_DEBUG_DECODER

#define TAG_RD  SELF_PROTECT(decodeInfo.rd_set, "decodeInfo.rd is not set by decoder.");
#define TAG_RS1 SELF_PROTECT(decodeInfo.rs1_set, "decodeInfo.rs1 is not set by decoder.");
#define TAG_RS2 SELF_PROTECT(decodeInfo.rs2_set, "decodeInfo.rs2 is not set by decoder.");
#define TAG_IMM SELF_PROTECT(decodeInfo.imm_set, "decodeInfo.imm is not set by decoder.");
#define TAG_CSR SELF_PROTECT(decodeInfo.csr_set, "decodeInfo.csr is not set by decoder.");

#else

#define TAG_RD  
#define TAG_RS1
#define TAG_RS2
#define TAG_IMM
#define TAG_CSR

#endif
