#include "cpu/riscv/core.hpp"
#include "cpu/word.hpp"
#include "macro.h"

#include <cstdint>

#include "./local-decoder.h"

#ifdef KXEMU_ISA64
    #define SHAMT_MASK 0x3f
#else
    #define SHAMT_MASK 0x1f
#endif

// #define RV64ONLY do { if (this->mode32) { this->do_invalid_inst(); return; }} while(0);
#define RV64ONLY

using namespace kxemu::cpu;

#define DO_INST_R(name, op) \
void RVCore::do_##name(const DecodeInfo &decodeInfo) { \
    TAG_RD; TAG_RS1; TAG_RS2;\
    \
    DEST = SRC1 op SRC2; \
} \

DO_INST_R(add, +)
DO_INST_R(sub, -)
DO_INST_R(and, &)
DO_INST_R(or , |)
DO_INST_R(xor, ^)

void RVCore::do_sll(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2;
    
    DEST = SRC1 << (SRC2 & SHAMT_MASK);
}

void RVCore::do_srl(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2;
    
    DEST = SRC1 >> (SRC2 & SHAMT_MASK);
}

void RVCore::do_sra(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2;
    
    DEST = (sword_t)SRC1 >> (SRC2 & SHAMT_MASK);
}

void RVCore::do_slt(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2;
    
    DEST = (sword_t)SRC1 < (sword_t)SRC2 ? 1 : 0;
}

void RVCore::do_sltu(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2;
    
    DEST = (word_t)SRC1 < (word_t)SRC2 ? 1 : 0;
}

#ifdef KXEMU_ISA64

void RVCore::do_addw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2;
    
    DEST = SEXT64((int32_t)SRC1 + (int32_t)SRC2);
}

void RVCore::do_subw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2;
    
    DEST = SEXT64((int32_t)SRC1 - (int32_t)SRC2);
}

void RVCore::do_sllw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2;
    
    DEST = SEXT64((uint32_t)SRC1) << (SRC2 & 0x1f);
}

void RVCore::do_srlw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2;
    
    DEST = SEXT64(((uint32_t)SRC1) >> (SRC2 & 0x1f));
}

void RVCore::do_sraw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_RS2;
    
    DEST = SEXT64((int32_t)SRC1) >> (SRC2 & 0x1f);
}

#endif

#define DO_INST_I(name, op) \
void RVCore::do_##name(const DecodeInfo &decodeInfo) { \
    TAG_RD; TAG_RS1; TAG_IMM; \
    \
    DEST = SRC1 op IMM; \
} \
// DO_INST_I(addi, +) expand to:
// void RVCore::do_addi(const DecodeInfo &decodeInfo) {
//     TAG_RD; TAG_RS1; TAG_IMM;
//     
//     DEST = SRC1 op IMM;      
// }

DO_INST_I(addi, +);
DO_INST_I(andi, &);
DO_INST_I(ori , |);
DO_INST_I(xori, ^);

void RVCore::do_slli(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    
    DEST = SRC1 << IMM;
}

void RVCore::do_srli(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    
    DEST = SRC1 >> IMM;
}

void RVCore::do_srai(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    
    DEST = (sword_t)SRC1 >> IMM;
}

void RVCore::do_slti(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    
    DEST = (sword_t)SRC1 < (sword_t)IMM ? 1 : 0;
}

void RVCore::do_sltiu(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    
    DEST = (word_t)SRC1 < (word_t)IMM ? 1 : 0;
}

#ifdef KXEMU_ISA64

void RVCore::do_addiw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    
    DEST = SEXT64((int32_t)SRC1 + (int32_t)IMM);
}

void RVCore::do_slliw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    
    DEST = SEXT64(((uint32_t)SRC1) << IMM);
}

void RVCore::do_srliw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    
    DEST = SEXT64(((uint32_t)SRC1) >> IMM);
}

void RVCore::do_sraiw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    
    DEST = SEXT64(((int32_t)SRC1) >> IMM);
}

#endif

// #define ADDR (MODE32 ? (this->gpr[rs1] + IMM) & 0xffffffff : this->gpr[rs1] + IMM)
#define ADDR (SRC1 + IMM)

void RVCore::do_lb(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;

    DEST = (sword_t)(int8_t)this->memory_load(ADDR, 1); 
}

void RVCore::do_lbu(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;

    DEST = this->memory_load(ADDR, 1);
}

void RVCore::do_lh(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    
    DEST = (sword_t)(int16_t)this->memory_load(ADDR, 2);
}

void RVCore::do_lhu(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;

    DEST = this->memory_load(ADDR, 2);
}

void RVCore::do_lw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;

    DEST = (sword_t)(int32_t)this->memory_load(ADDR, 4);
}

#ifdef KXEMU_ISA64

void RVCore::do_lwu(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    RV64ONLY;
    
    DEST = this->memory_load(ADDR, 4);
}

void RVCore::do_ld(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    RV64ONLY;
    
    DEST = this->memory_load(ADDR, 8);
}

#endif

void RVCore::do_sb(const DecodeInfo &decodeInfo) {
    TAG_RS1; TAG_RS2; TAG_IMM;
    
    this->memory_store(ADDR, SRC2, 1);
}

void RVCore::do_sh(const DecodeInfo &decodeInfo) {
    TAG_RS1; TAG_RS2; TAG_IMM;
    
    this->memory_store(ADDR, SRC2, 2);
}

void RVCore::do_sw(const DecodeInfo &decodeInfo) {
    TAG_RS1; TAG_RS2; TAG_IMM;
    
    this->memory_store(ADDR, SRC2, 4);
}

#ifdef KXEMU_ISA64
void RVCore::do_sd(const DecodeInfo &decodeInfo) {
    TAG_RS1; TAG_RS2; TAG_IMM;
    RV64ONLY;
    
    this->memory_store(ADDR, SRC2, 8);
}
#endif

// #define COND(op) (MODE32 ? ((int32_t)this->gpr[rs1] op (int32_t)this->gpr[rs2]) : ((sword_t)this->gpr[rs1] op (sword_t)this->gpr[rs2]))
// #define COND_U(op) (MODE32 ? ((uint32_t)this->gpr[rs1] op (uint32_t)this->gpr[rs2]) : (this->gpr[rs1] op this->gpr[rs2]))
#define COND(op) unlikely((sword_t)SRC1 op (sword_t)SRC2)
#define COND_U(op) unlikely((word_t)SRC1 op (word_t)SRC2)

#define DO_INST_B(name, op) \
void RVCore::do_##name(const DecodeInfo &decodeInfo) { \
    TAG_RS1; TAG_RS2; TAG_NPC; \
    \
    if (unlikely((sword_t)SRC1 op (sword_t)SRC2)) { \
        this->npc = NPC; \
    } \
} \

#define DO_INST_BU(name, op) \
void RVCore::do_##name(const DecodeInfo &decodeInfo) { \
    TAG_RS1; TAG_RS2; TAG_NPC; \
    \
    if (unlikely(SRC1 op SRC2)) { \
        this->npc = NPC; \
    } \
} \

// DO_INST_B(beq , ==) expand to:
// void RVCore::do_beq(const DecodeInfo &decodeInfo) {
//     // RS1; RS2; IMM;
//     if (COND(==)) {
//         this->npc = this->pc + IMM;
//     }
// }

// DO_INST_BU(bgeu, >=) expand to:
// void RVCore::do_bgeu(const DecodeInfo &decodeInfo) {
//     // RS1; RS2; IMM;
//     if (COND_U(>=)) {
//         this->npc = this->pc + IMM;
//     }
// }

DO_INST_B (beq , ==);
DO_INST_B (bne , !=);
DO_INST_B (bge , >=);
DO_INST_BU(bgeu, >=);
DO_INST_B (blt , < );
DO_INST_BU(bltu, < );

void RVCore::do_jal(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    DEST = this->pc + 4;
    this->npc = this->pc + IMM;
}

void RVCore::do_jalr(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    
    this->npc = SRC1 + IMM;
    DEST = this->pc + 4;
}

void RVCore::do_lui(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    DEST = IMM;
}

void RVCore::do_auipc(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    DEST = IMM;
}
