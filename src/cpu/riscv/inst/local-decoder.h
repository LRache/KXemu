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
#define NPC  ( decodeInfo.npc)

#define FDESTS (this->fpr[decodeInfo.rd ].f32)
#define FSRC1S (this->fpr[decodeInfo.rs1].f32)
#define FSRC2S (this->fpr[decodeInfo.rs2].f32)
#define FSRC3S (this->fpr[decodeInfo.rs3].f32)

#define FDESTD (this->fpr[decodeInfo.rd ].f64)
#define FSRC1D (this->fpr[decodeInfo.rs1].f64)
#define FSRC2D (this->fpr[decodeInfo.rs2].f64)
#define FSRC3D (this->fpr[decodeInfo.rs3].f64)

#define FPR_FILL_DEST_HIGH this->fpr[decodeInfo.rd].high = -1;

// #define  rd_is_x0 (decodeInfo.rd  == &this->gpr[32])
// #define rs1_is_x0 (decodeInfo.rs1 == &this->gpr[ 0])
#define  rd_is_x0 (decodeInfo.rd  == 32)
#define rs1_is_x0 (decodeInfo.rs1 ==  0)

#ifdef CONFIG_DEBUG_DECODER

#define TAG_RD  Assert(decodeInfo.rd_set, "decodeInfo.rd is not set by decoder.");
#define TAG_RS1 Assert(decodeInfo.rs1_set, "decodeInfo.rs1 is not set by decoder.");
#define TAG_RS2 Assert(decodeInfo.rs2_set, "decodeInfo.rs2 is not set by decoder.");
#define TAG_IMM Assert(decodeInfo.imm_set, "decodeInfo.imm is not set by decoder.");
#define TAG_CSR Assert(decodeInfo.csr_set, "decodeInfo.csr is not set by decoder.");
#define TAG_NPC Assert(decodeInfo.npc_set, "decodeInfo.npc is not set by decoder.");

#else

#define TAG_RD  
#define TAG_RS1
#define TAG_RS2
#define TAG_IMM
#define TAG_CSR
#define TAG_NPC
#define TAG_FRD

#endif
