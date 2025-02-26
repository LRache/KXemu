#include "cpu/riscv/core.h"
#include "cpu/word.h"

#include "./local-decoder.h"

#define RD  unsigned int rd  = decodeInfo.rd
#define RS1 unsigned int rs1 = decodeInfo.rs1
#define RS2 unsigned int rs2 = decodeInfo.rs2
#define IMM word_t imm = decodeInfo.imm

// #define ADDR (MODE32 ? (this->gpr[rs1] + imm) & 0xffffffff : this->gpr[rs1] + imm)
#define ADDR (this->gpr[rs1] + imm)
// #define RV64ONLY do { if (this->mode32) { this->do_invalid_inst(); return; }} while(0);
#define RV64ONLY

using namespace kxemu::cpu;

void RVCore::do_add(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] + this->gpr[rs2]);
}

void RVCore::do_sub(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] - this->gpr[rs2]);
}

void RVCore::do_and(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] & this->gpr[rs2]);
}

void RVCore::do_or(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] | this->gpr[rs2]);
}

void RVCore::do_xor(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] ^ this->gpr[rs2]);
}

void RVCore::do_sll(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] << (this->gpr[rs2] & 0x1F));
}

void RVCore::do_srl(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] >> (this->gpr[rs2] & 0x1F));
}

void RVCore::do_sra(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] >> (this->gpr[rs2] & 0x1F));
}

void RVCore::do_slt(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] < (sword_t)this->gpr[rs2] ? 1 : 0);
}

void RVCore::do_sltu(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] < this->gpr[rs2] ? 1 : 0);
}

#ifdef KXEMU_ISA64

void RVCore::do_addw(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    int32_t result = (int32_t)this->gpr[rs1] + (int32_t)this->gpr[rs2];
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_subw(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    int32_t result = (int32_t)this->gpr[rs1] - (int32_t)this->gpr[rs2];
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_sllw(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    uint32_t result = ((uint32_t)this->gpr[rs1]) << (this->gpr[rs2] & 0x1f);
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_srlw(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    uint32_t result = ((uint32_t)this->gpr[rs1]) >> (this->gpr[rs2] & 0x1f);
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_sraw(const DecodeInfo &deocdeInfo) {
    RD; RS1; RS2;
    int32_t result = ((int32_t)this->gpr[rs1]) >> (this->gpr[rs2] & 0x1f);
    this->set_gpr(rd, (word_t)result);
}

#endif

void RVCore::do_addi(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, this->gpr[rs1] + imm);        
}

void RVCore::do_andi(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, this->gpr[rs1] & imm);
}

void RVCore::do_ori(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, this->gpr[rs1] | imm);
}

void RVCore::do_xori(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, this->gpr[rs1] ^ imm);
}

#ifdef KXEMU_ISA64
    #define SHAMT_MASK 0x3f
#else
    #define SHAMT_MASK 0x1f
#endif

void RVCore::do_slli(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, this->gpr[rs1] << (imm & SHAMT_MASK));
}

void RVCore::do_srli(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, this->gpr[rs1] >> (imm & SHAMT_MASK));
}

void RVCore::do_srai(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] >> (imm & SHAMT_MASK));
}

void RVCore::do_slti(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] < (sword_t)imm ? 1 : 0);
}

void RVCore::do_sltiu(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, this->gpr[rs1] < (word_t)imm ? 1 : 0);
}

#ifdef KXEMU_ISA64

void RVCore::do_addiw(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    int32_t result = (int32_t)this->gpr[rs1] + imm;
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_slliw(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    int32_t result = ((uint32_t)this->gpr[rs1]) << (imm & 0x1f);
    this->set_gpr(rd, (sword_t)result);
}

void RVCore::do_srliw(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    int32_t result = ((uint32_t)this->gpr[rs1]) >> (imm & 0x1f);
    this->set_gpr(rd, (sword_t)result);
}

void RVCore::do_sraiw(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    int32_t result = ((int32_t)this->gpr[rs1]) >> (imm & 0x1f);
    this->set_gpr(rd, (sword_t)result);
}

#endif

void RVCore::do_lb(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, (sword_t)(int8_t)this->memory_load(ADDR, 1));
}

void RVCore::do_lbu(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, this->memory_load(ADDR, 1));
}

void RVCore::do_lh(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, (sword_t)(int16_t)this->memory_load(ADDR, 2));
}

void RVCore::do_lhu(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, this->memory_load(ADDR, 2));
}

void RVCore::do_lw(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, (sword_t)(int32_t)this->memory_load(ADDR, 4));
}

#ifdef KXEMU_ISA64
void RVCore::do_lwu(const DecodeInfo &deocdeInfo) {
    RV64ONLY;
    RD; RS1; IMM;
    this->set_gpr(rd, this->memory_load(ADDR, 4));
}

void RVCore::do_ld(const DecodeInfo &deocdeInfo) {
    RV64ONLY;
    RD; RS1; IMM;
    word_t data = this->memory_load(this->gpr[rs1] + imm, 8);
    this->set_gpr(rd, data);
}
#endif

void RVCore::do_sb(const DecodeInfo &deocdeInfo) {
    RS1; RS2; IMM;
    this->memory_store(ADDR, this->gpr[rs2], 1);
}

void RVCore::do_sh(const DecodeInfo &deocdeInfo) {
    RS1; RS2; IMM;
    this->memory_store(ADDR, this->gpr[rs2], 2);
}

void RVCore::do_sw(const DecodeInfo &deocdeInfo) {
    RS1; RS2; IMM;
    this->memory_store(ADDR, this->gpr[rs2], 4);
}

#ifdef KXEMU_ISA64
void RVCore::do_sd(const DecodeInfo &deocdeInfo) {
    RV64ONLY;
    RS1; RS2; IMM;
    this->memory_store(this->gpr[rs1] + imm, this->gpr[rs2], 8);
}
#endif

// #define COND(op) (MODE32 ? ((int32_t)this->gpr[rs1] op (int32_t)this->gpr[rs2]) : ((sword_t)this->gpr[rs1] op (sword_t)this->gpr[rs2]))
// #define COND_U(op) (MODE32 ? ((uint32_t)this->gpr[rs1] op (uint32_t)this->gpr[rs2]) : (this->gpr[rs1] op this->gpr[rs2]))
#define COND(op) ((sword_t)this->gpr[rs1] op (sword_t)this->gpr[rs2])
#define COND_U(op) (this->gpr[rs1] op this->gpr[rs2])

void RVCore::do_beq(const DecodeInfo &deocdeInfo) {
    RS1; RS2; IMM;
    if (COND(==)) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_bge(const DecodeInfo &deocdeInfo) {
    RS1; RS2; IMM;
    if (COND(>=)) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_bgeu(const DecodeInfo &deocdeInfo) {
    RS1; RS2; IMM;
    if (COND_U(>=)) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_blt(const DecodeInfo &deocdeInfo) {
    RS1; RS2; IMM;
    if (COND(<)) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_bltu(const DecodeInfo &deocdeInfo) {
    RS1; RS2; IMM;
    if (COND_U(<)) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_bne(const DecodeInfo &deocdeInfo) {
    RS1; RS2; IMM;
    if (COND(!=)) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_jal(const DecodeInfo &deocdeInfo) {
    RD; IMM;
    this->set_gpr(rd, this->pc + 4);
    this->npc = this->pc + imm;
}

void RVCore::do_jalr(const DecodeInfo &deocdeInfo) {
    RD; RS1; IMM;
    this->set_gpr(rd, this->pc + 4);
    this->npc = this->gpr[rs1] + imm;
}

void RVCore::do_lui(const DecodeInfo &deocdeInfo) {
    RD; IMM;
    this->set_gpr(rd, imm);
}

void RVCore::do_auipc(const DecodeInfo &deocdeInfo) {
    RD; IMM;
    this->set_gpr(rd, this->pc + imm);
}
