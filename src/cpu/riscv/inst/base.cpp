#include "cpu/riscv/core.h"
#include "cpu/word.h"

#include "./local-decoder.h"
#include "utils/gdb-rsp.h"
#include <cstdint>

#ifdef KXEMU_ISA64
    #define SHAMT_MASK 0x3f
#else
    #define SHAMT_MASK 0x1f
#endif

// #define ADDR (MODE32 ? (this->gpr[rs1] + IMM) & 0xffffffff : this->gpr[rs1] + IMM)
// #define ADDR (this->get_gpr_core(rs1) + IMM)
// #define RV64ONLY do { if (this->mode32) { this->do_invalid_inst(); return; }} while(0);
#define RV64ONLY

using namespace kxemu::cpu;

#define DO_INST_R(name, op) \
void RVCore::do_##name(const DecodeInfo &decodeInfo) { \
    DEST = SRC1 op SRC2; \
} \

DO_INST_R(add, +)
DO_INST_R(sub, -)
DO_INST_R(and, &)
DO_INST_R(or , |)
DO_INST_R(xor, ^)

void RVCore::do_sll(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // this->set_gpr(rd, this->gpr[rs1] << (this->gpr[rs2] & SHAMT_MASK));
    DEST = SRC1 << (SRC2 & SHAMT_MASK);
}

void RVCore::do_srl(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // this->set_gpr(rd, this->gpr[rs1] >> (this->gpr[rs2] & SHAMT_MASK));
    DEST = SRC1 >> (SRC2 & SHAMT_MASK);
}

void RVCore::do_sra(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // this->set_gpr(rd, (sword_t)this->gpr[rs1] >> (this->gpr[rs2] & SHAMT_MASK));
    DEST = (sword_t)SRC1 >> (SRC2 & SHAMT_MASK);
}

void RVCore::do_slt(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // this->set_gpr(rd, (sword_t)this->gpr[rs1] < (sword_t)this->gpr[rs2] ? 1 : 0);
    DEST = (sword_t)SRC1 < (sword_t)SRC2 ? 1 : 0;
}

void RVCore::do_sltu(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // this->set_gpr(rd, this->gpr[rs1] < this->gpr[rs2] ? 1 : 0);
    DEST = (word_t)SRC1 < (word_t)SRC2 ? 1 : 0;
}

#ifdef KXEMU_ISA64

void RVCore::do_addw(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // int32_t result = (int32_t)this->gpr[rs1] + (int32_t)this->gpr[rs2];
    // this->set_gpr(rd, (word_t)result);
    DEST = SEXT64((int32_t)SRC1 + (int32_t)SRC2);
}

void RVCore::do_subw(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // int32_t result = (int32_t)this->gpr[rs1] - (int32_t)this->gpr[rs2];
    // this->set_gpr(rd, (word_t)result);
    DEST = SEXT64((int32_t)SRC1 - (int32_t)SRC2);
}

void RVCore::do_sllw(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // uint32_t result = ((uint32_t)this->gpr[rs1]) << (this->gpr[rs2] & 0x1f);
    // this->set_gpr(rd, (word_t)result);
    DEST = SEXT64((uint32_t)SRC1) << (SRC2 & 0x1f);
}

void RVCore::do_srlw(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // uint32_t result = ((uint32_t)this->gpr[rs1]) >> (this->gpr[rs2] & 0x1f);
    // this->set_gpr(rd, (word_t)result);
    DEST = SEXT64(((uint32_t)SRC1) >> (SRC2 & 0x1f));
}

void RVCore::do_sraw(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // int32_t result = ((int32_t)this->gpr[rs1]) >> (this->gpr[rs2] & 0x1f);
    // this->set_gpr(rd, (word_t)result);
    DEST = SEXT64((int32_t)SRC1) >> (SRC2 & 0x1f);
}

#endif

#define DO_INST_I(name, op) \
void RVCore::do_##name(const DecodeInfo &decodeInfo) { \
    DEST = SRC1 op IMM; \
} \

DO_INST_I(addi, +);
DO_INST_I(andi, &);
DO_INST_I(ori , |);
DO_INST_I(xori, ^);

// void RVCore::do_addi(const DecodeInfo &decodeInfo) {
//     RD; RS1; IMM;
//     this->set_gpr(rd, this->gpr[rs1] + IMM);        
// }

// void RVCore::do_andi(const DecodeInfo &decodeInfo) {
//     RD; RS1; IMM;
//     this->set_gpr(rd, this->gpr[rs1] & IMM);
// }

// void RVCore::do_ori(const DecodeInfo &decodeInfo) {
//     RD; RS1; IMM;
//     this->set_gpr(rd, this->gpr[rs1] | IMM);
// }

// void RVCore::do_xori(const DecodeInfo &decodeInfo) {
//     RD; RS1; IMM;
//     this->set_gpr(rd, this->gpr[rs1] ^ IMM);
// }

void RVCore::do_slli(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // this->set_gpr(rd, this->gpr[rs1] << (IMM & SHAMT_MASK));
    DEST = SRC1 << (IMM & SHAMT_MASK);
}

void RVCore::do_srli(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // this->set_gpr(rd, this->gpr[rs1] >> (IMM & SHAMT_MASK));
    DEST = SRC1 >> (IMM & SHAMT_MASK);
}

void RVCore::do_srai(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // this->set_gpr(rd, (sword_t)this->gpr[rs1] >> (IMM & SHAMT_MASK));
    DEST = (sword_t)SRC1 >> (IMM & SHAMT_MASK);
}

void RVCore::do_slti(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // this->set_gpr(rd, (sword_t)this->gpr[rs1] < (sword_t)IMM ? 1 : 0);
    DEST = (sword_t)SRC1 < (sword_t)IMM ? 1 : 0;
}

void RVCore::do_sltiu(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // this->set_gpr(rd, this->gpr[rs1] < (word_t)IMM ? 1 : 0);
    DEST = (word_t)SRC1 < (word_t)IMM ? 1 : 0;
}

#ifdef KXEMU_ISA64

void RVCore::do_addiw(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // int32_t result = (int32_t)this->gpr[rs1] + IMM;
    // this->set_gpr(rd, (word_t)result);
    DEST = SEXT64((int32_t)SRC1 + (int32_t)IMM);
}

void RVCore::do_slliw(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // int32_t result = ((uint32_t)this->gpr[rs1]) << (IMM & 0x1f);
    // this->set_gpr(rd, (sword_t)result);
    DEST = SEXT64(((uint32_t)SRC1) << (IMM & 0x1f));
}

void RVCore::do_srliw(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // int32_t result = ((uint32_t)this->gpr[rs1]) >> (IMM & 0x1f);
    // this->set_gpr(rd, (sword_t)result);
    DEST = SEXT64(((uint32_t)SRC1) >> (IMM & 0x1f));
}

void RVCore::do_sraiw(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // int32_t result = ((int32_t)this->gpr[rs1]) >> (IMM & 0x1f);
    // this->set_gpr(rd, (sword_t)result);
    DEST = SEXT64(((int32_t)SRC1) >> (IMM & 0x1f));
}

#endif

#define ADDR (SRC1 + IMM)

void RVCore::do_lb(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // this->set_gpr(rd, (sword_t)(int8_t)this->memory_load(ADDR, 1));
    DEST = (sword_t)(int8_t)this->memory_load(ADDR, 1);
}

void RVCore::do_lbu(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // this->set_gpr(rd, this->memory_load(ADDR, 1));
    DEST = this->memory_load(ADDR, 1);
}

void RVCore::do_lh(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // this->set_gpr(rd, (sword_t)(int16_t)this->memory_load(ADDR, 2));
    DEST = (sword_t)(int16_t)this->memory_load(ADDR, 2);
}

void RVCore::do_lhu(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // this->set_gpr(rd, this->memory_load(ADDR, 2));
    DEST = this->memory_load(ADDR, 2);
}

void RVCore::do_lw(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // this->set_gpr_core(rd, (sword_t)(int32_t)this->memory_load(ADDR, 4));
    DEST = (sword_t)(int32_t)this->memory_load(ADDR, 4);
}

#ifdef KXEMU_ISA64
void RVCore::do_lwu(const DecodeInfo &decodeInfo) {
    RV64ONLY;
    // RD; RS1; IMM;
    // this->set_gpr(rd, this->memory_load(ADDR, 4));
    DEST = this->memory_load(ADDR, 4);
}

void RVCore::do_ld(const DecodeInfo &decodeInfo) {
    RV64ONLY;
    // RD; RS1; IMM;
    // word_t data = this->memory_load(this->gpr[rs1] + IMM, 8);
    // this->set_gpr(rd, data);
    DEST = this->memory_load(ADDR, 8);
}
#endif

void RVCore::do_sb(const DecodeInfo &decodeInfo) {
    // RS1; RS2; IMM;
    // this->memory_store(ADDR, this->gpr[rs2], 1);
    this->memory_store(ADDR, SRC2, 1);
}

void RVCore::do_sh(const DecodeInfo &decodeInfo) {
    // RS1; RS2; IMM;
    // this->memory_store(ADDR, this->gpr[rs2], 2);
    this->memory_store(ADDR, SRC2, 2);
}

void RVCore::do_sw(const DecodeInfo &decodeInfo) {
    // RS1; RS2; IMM;
    // this->memory_store(ADDR, this->gpr[rs2], 4);
    this->memory_store(ADDR, SRC2, 4);
}

#ifdef KXEMU_ISA64
void RVCore::do_sd(const DecodeInfo &decodeInfo) {
    RV64ONLY;
    // RS1; RS2; IMM;
    // this->memory_store(this->get_gpr_core(rs1) + IMM, this->get_gpr_core(rs2), 8);
    this->memory_store(ADDR, SRC2, 8);
}
#endif

// #define COND(op) (MODE32 ? ((int32_t)this->gpr[rs1] op (int32_t)this->gpr[rs2]) : ((sword_t)this->gpr[rs1] op (sword_t)this->gpr[rs2]))
// #define COND_U(op) (MODE32 ? ((uint32_t)this->gpr[rs1] op (uint32_t)this->gpr[rs2]) : (this->gpr[rs1] op this->gpr[rs2]))
#define COND(op) ((sword_t)this->gpr[rs1] op (sword_t)this->gpr[rs2])
#define COND_U(op) (this->gpr[rs1] op this->gpr[rs2])

#define DO_INST_B(name, op) \
void RVCore::do_##name(const DecodeInfo &decodeInfo) { \
    if (unlikely((sword_t)SRC1 op (sword_t)SRC2)) { \
        this->npc = this->pc + IMM; \
    } \
} \

#define DO_INST_BU(name, op) \
void RVCore::do_##name(const DecodeInfo &decodeInfo) { \
    if (unlikely(SRC1 op SRC2)) { \
        this->npc = this->pc + IMM; \
    } \
} \

DO_INST_B (beq , ==);
DO_INST_B (bne , !=);
DO_INST_B (bge , >=);
DO_INST_BU(bgeu, >=);
DO_INST_B (blt , < );
DO_INST_BU(bltu, < );

// void RVCore::do_beq(const DecodeInfo &decodeInfo) {
//     RS1; RS2; IMM;
//     if (COND(==)) {
//         this->npc = this->pc + IMM;
//     }
// }

// void RVCore::do_bge(const DecodeInfo &decodeInfo) {
//     RS1; RS2; IMM;
//     if (COND(>=)) {
//         this->npc = this->pc + IMM;
//     }
// }

// void RVCore::do_bgeu(const DecodeInfo &decodeInfo) {
//     RS1; RS2; IMM;
//     if (COND_U(>=)) {
//         this->npc = this->pc + IMM;
//     }
// }

// void RVCore::do_blt(const DecodeInfo &decodeInfo) {
//     RS1; RS2; IMM;
//     if (COND(<)) {
//         this->npc = this->pc + IMM;
//     }
// }

// void RVCore::do_bltu(const DecodeInfo &decodeInfo) {
//     RS1; RS2; IMM;
//     if (COND_U(<)) {
//         this->npc = this->pc + IMM;
//     }
// }

// void RVCore::do_bne(const DecodeInfo &decodeInfo) {
//     RS1; RS2; IMM;
//     if (COND(!=)) {
//         this->npc = this->pc + IMM;
//     }
// }

void RVCore::do_jal(const DecodeInfo &decodeInfo) {
    // RD; IMM;
    // this->set_gpr(rd, this->pc + 4);
    // this->npc = this->pc + IMM;
    DEST = this->pc + 4;
    this->npc = this->pc + IMM;
}

void RVCore::do_jalr(const DecodeInfo &decodeInfo) {
    // RD; RS1; IMM;
    // this->set_gpr(rd, this->pc + 4);
    // this->npc = this->gpr[rs1] + IMM;
    DEST = this->pc + 4;
    this->npc = SRC1 + IMM;
}

void RVCore::do_lui(const DecodeInfo &decodeInfo) {
    // RD; IMM;
    // this->set_gpr(rd, IMM);
    DEST = IMM;
}

void RVCore::do_auipc(const DecodeInfo &decodeInfo) {
    // RD; IMM;
    // this->set_gpr(rd, this->pc + IMM);
    DEST = this->pc + IMM;
}
