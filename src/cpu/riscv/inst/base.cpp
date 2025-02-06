#include "cpu/riscv/core.h"

#include <cstdint>

#include "./local-decoder.h"
#include "cpu/word.h"

#define RD  unsigned int rd  = BITS(11, 7);
#define RS1 unsigned int rs1 = BITS(19, 15);
#define RS2 unsigned int rs2 = BITS(24, 20);
#define IMM_I sword_t imm = (sword_t)SEXT(BITS(31, 20), 12);
#define IMM_S sword_t imm = (sword_t)SEXT((BITS(31, 25) << 5) | BITS(11, 7), 12);
#define IMM_B sword_t imm = (sword_t)SEXT((BITS(31, 31) << 12) | (BITS(30, 25) << 5) | (BITS(11, 8) << 1) | (BITS(7, 7) << 11), 13);
#define IMM_J sword_t imm = (sword_t)SEXT((BITS(31, 31) << 20) | (BITS(30, 21) << 1) | (BITS(20, 20) << 11) | (BITS(19, 12) << 12), 21);
#define IMM_U sword_t imm = (sword_t)(int32_t)(BITS(31, 12) << 12);

// #define ADDR (MODE32 ? (this->gpr[rs1] + imm) & 0xffffffff : this->gpr[rs1] + imm)
#define ADDR (this->gpr[rs1] + imm)
// #define RV64ONLY do { if (this->mode32) { this->do_invalid_inst(); return; }} while(0);
#define RV64ONLY

using namespace kxemu::cpu;

void RVCore::do_add() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] + this->gpr[rs2]);
}

void RVCore::do_sub() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] - this->gpr[rs2]);
}

void RVCore::do_and() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] & this->gpr[rs2]);
}

void RVCore::do_or() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] | this->gpr[rs2]);
}

void RVCore::do_xor() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] ^ this->gpr[rs2]);
}

void RVCore::do_sll() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] << (this->gpr[rs2] & 0x1F));
}

void RVCore::do_srl() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] >> (this->gpr[rs2] & 0x1F));
}

void RVCore::do_sra() {
    RD; RS1; RS2;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] >> (this->gpr[rs2] & 0x1F));
}

void RVCore::do_slt() {
    RD; RS1; RS2;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] < (sword_t)this->gpr[rs2] ? 1 : 0);
}

void RVCore::do_sltu() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] < this->gpr[rs2] ? 1 : 0);
}

#ifdef KXEMU_ISA64

void RVCore::do_addw() {
    RD; RS1; RS2;
    int32_t result = (int32_t)this->gpr[rs1] + (int32_t)this->gpr[rs2];
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_subw() {
    RD; RS1; RS2;
    int32_t result = (int32_t)this->gpr[rs1] - (int32_t)this->gpr[rs2];
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_sllw() {
    RD; RS1; RS2;
    uint32_t result = ((uint32_t)this->gpr[rs1]) << (this->gpr[rs2] & 0x1f);
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_srlw() {
    RD; RS1; RS2;
    uint32_t result = ((uint32_t)this->gpr[rs1]) >> (this->gpr[rs2] & 0x1f);
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_sraw() {
    RD; RS1; RS2;
    int32_t result = ((int32_t)this->gpr[rs1]) >> (this->gpr[rs2] & 0x1f);
    this->set_gpr(rd, (word_t)result);
}

#endif

void RVCore::do_addi() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] + imm);        
}

void RVCore::do_andi() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] & imm);
}

void RVCore::do_ori() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] | imm);
}

void RVCore::do_xori() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] ^ imm);
}

#ifdef KXEMU_ISA64
    #define SHAMT_MASK 0x3f
#else
    #define SHAMT_MASK 0x1f
#endif

void RVCore::do_slli() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] << (imm & SHAMT_MASK));
}

void RVCore::do_srli() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] >> (imm & SHAMT_MASK));
}

void RVCore::do_srai() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] >> (imm & SHAMT_MASK));
}

void RVCore::do_slti() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] < (sword_t)imm ? 1 : 0);
}

void RVCore::do_sltiu() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] < (word_t)imm ? 1 : 0);
}

#ifdef KXEMU_ISA64

void RVCore::do_addiw() {
    RD; RS1; IMM_I;
    int32_t result = (int32_t)this->gpr[rs1] + imm;
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_slliw() {
    RD; RS1; IMM_I;
    int32_t result = ((uint32_t)this->gpr[rs1]) << (imm & 0x1f);
    this->set_gpr(rd, (sword_t)result);
}

void RVCore::do_srliw() {
    RD; RS1; IMM_I;
    int32_t result = ((uint32_t)this->gpr[rs1]) >> (imm & 0x1f);
    this->set_gpr(rd, (sword_t)result);
}

void RVCore::do_sraiw() {
    RD; RS1; IMM_I;
    int32_t result = ((int32_t)this->gpr[rs1]) >> (imm & 0x1f);
    this->set_gpr(rd, (sword_t)result);
}

#endif

void RVCore::do_lb() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, (sword_t)(int8_t)this->memory_load(ADDR, 1));
}

void RVCore::do_lbu() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->memory_load(ADDR, 1));
}

void RVCore::do_lh() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, (sword_t)(int16_t)this->memory_load(ADDR, 2));
}

void RVCore::do_lhu() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->memory_load(ADDR, 2));
}

void RVCore::do_lw() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, (sword_t)(int32_t)this->memory_load(ADDR, 4));
}

#ifdef KXEMU_ISA64
void RVCore::do_lwu() {
    RV64ONLY;
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->memory_load(ADDR, 4));
}

void RVCore::do_ld() {
    RV64ONLY;
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->memory_load(this->gpr[rs1] + imm, 8));
}
#endif

void RVCore::do_sb() {
    RS1; RS2; IMM_S;
    this->memory_store(ADDR, this->gpr[rs2], 1);
}

void RVCore::do_sh() {
    RS1; RS2; IMM_S;
    this->memory_store(ADDR, this->gpr[rs2], 2);
}

void RVCore::do_sw() {
    RS1; RS2; IMM_S;
    this->memory_store(ADDR, this->gpr[rs2], 4);
}

#ifdef KXEMU_ISA64
void RVCore::do_sd() {
    RV64ONLY;
    RS1; RS2; IMM_S;
    this->memory_store(this->gpr[rs1] + imm, this->gpr[rs2], 8);
}
#endif

// #define COND(op) (MODE32 ? ((int32_t)this->gpr[rs1] op (int32_t)this->gpr[rs2]) : ((sword_t)this->gpr[rs1] op (sword_t)this->gpr[rs2]))
// #define COND_U(op) (MODE32 ? ((uint32_t)this->gpr[rs1] op (uint32_t)this->gpr[rs2]) : (this->gpr[rs1] op this->gpr[rs2]))
#define COND(op) ((sword_t)this->gpr[rs1] op (sword_t)this->gpr[rs2])
#define COND_U(op) (this->gpr[rs1] op this->gpr[rs2])

void RVCore::do_beq() {
    RS1; RS2; IMM_B;
    if (COND(==)) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_bge() {
    RS1; RS2; IMM_B;
    if (COND(>=)) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_bgeu() {
    RS1; RS2; IMM_B;
    if (COND_U(>=)) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_blt() {
    RS1; RS2; IMM_B;
    if (COND(<)) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_bltu() {
    RS1; RS2; IMM_B;
    if (COND_U(<)) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_bne() {
    RS1; RS2; IMM_B;
    if (COND(!=)) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_jal() {
    RD; IMM_J;
    this->set_gpr(rd, this->pc + 4);
    this->npc = this->pc + imm;
}

void RVCore::do_jalr() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->pc + 4);
    this->npc = this->gpr[rs1] + imm;
}

void RVCore::do_lui() {
    RD; IMM_U;
    this->set_gpr(rd, imm);
}

void RVCore::do_auipc() {
    RD; IMM_U;
    this->set_gpr(rd, this->pc + imm);
}
