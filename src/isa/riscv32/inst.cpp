#include "isa/riscv32/core.h"
#include "log.h"

#define INSTPAT(pat, name) this->decoder.add(pat, &RV32Core::do_##name)
#define RD  int rd  = get_rd (this->inst);
#define RS1 int rs1 = get_rs1(this->inst);
#define RS2 int rs2 = get_rs2(this->inst); 
#define IMM_I int32_t imm = (int32_t)sign_extend(sub_bits(this->inst, 31, 20), 12);
#define IMM_S int32_t imm = (int32_t)sign_extend(sub_bits(this->inst, 31, 25) << 5 | sub_bits(this->inst, 11, 7), 12);
#define IMM_B int32_t imm = (int32_t)sign_extend(sub_bits(this->inst, 31, 31) << 12 | sub_bits(this->inst, 30, 25) << 5 | sub_bits(this->inst, 11, 8) << 1 | sub_bits(this->inst, 7, 7) << 11, 13);
#define IMM_J int32_t imm = (int32_t)sign_extend(sub_bits(this->inst, 31, 31) << 20 | sub_bits(this->inst, 30, 21) << 1 | sub_bits(this->inst, 20, 20) << 11 | sub_bits(this->inst, 19, 12) << 12, 21);
#define IMM_U int32_t imm = (int32_t)sub_bits(this->inst, 31, 12) << 12;

static inline uint32_t sub_bits(uint64_t bits, int hi, int lo) {
    return (bits >> lo) & ((1 << (hi - lo + 1)) - 1);
}

static inline int get_rd(uint32_t inst) {
    return sub_bits(inst, 11, 7);
}

static inline int get_rs1(uint32_t inst) {
    return sub_bits(inst, 19, 15);
}

static inline int get_rs2(uint32_t inst) {
    return sub_bits(inst, 24, 20);
}

static inline int32_t sign_extend(uint32_t bits, int from) {
    int shift = 32 - from;
    return (int32_t)(bits << shift) >> shift;
}

void RV32Core::do_add() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] + this->gpr[rs2]);
}

void RV32Core::do_sub() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] - this->gpr[rs2]);
}

void RV32Core::do_and() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] & this->gpr[rs2]);
}

void RV32Core::do_or() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] | this->gpr[rs2]);
}

void RV32Core::do_xor() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] ^ this->gpr[rs2]);
}

void RV32Core::do_sll() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] << (this->gpr[rs2] & 0x1F));
}

void RV32Core::do_srl() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] >> (this->gpr[rs2] & 0x1F));
}

void RV32Core::do_sra() {
    RD; RS1; RS2;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] >> (this->gpr[rs2] & 0x1F));
}

void RV32Core::do_slt() {
    RD; RS1; RS2;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] < (sword_t)this->gpr[rs2] ? 1 : 0);
}

void RV32Core::do_sltu() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] < this->gpr[rs2] ? 1 : 0);
}

void RV32Core::do_addi() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] + imm);        
}

void RV32Core::do_andi() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] & imm);
}

void RV32Core::do_ori() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] | imm);
}

void RV32Core::do_xori() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] ^ imm);
}

void RV32Core::do_slli() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] << (imm & 0x1F));
}

void RV32Core::do_srli() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] >> (imm & 0x1F));
}

void RV32Core::do_srai() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] >> (imm & 0x1F));
}

void RV32Core::do_slti() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, (sword_t)this->gpr[rs1] < (sword_t)imm ? 1 : 0);
}

void RV32Core::do_sltiu() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->gpr[rs1] < (word_t)imm ? 1 : 0);
}

void RV32Core::do_lb() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, (int8_t)this->memory->read(this->gpr[rs1] + imm, 1));
}

void RV32Core::do_lbu() {
    RD; RS1; IMM_I;
    word_t data = this->memory_read(this->gpr[rs1] + imm, 1);
    this->set_gpr(rd, data);
}

void RV32Core::do_lh() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, (int16_t)this->memory_read(this->gpr[rs1] + imm, 2));
}

void RV32Core::do_lhu() {
    RD; RS1; IMM_I;
    word_t data = this->memory_read(this->gpr[rs1] + imm, 2);
    this->set_gpr(rd, data);
}

void RV32Core::do_lw() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->memory_read(this->gpr[rs1] + imm, 4));
}

void RV32Core::do_sb() {
    RS1; RS2; IMM_S;
    this->memory_write(this->gpr[rs1] + imm, this->gpr[rs2], 1);
}

void RV32Core::do_sh() {
    RS1; RS2; IMM_S;
    this->memory_write(this->gpr[rs1] + imm, this->gpr[rs2], 2);
}

void RV32Core::do_sw() {
    RS1; RS2; IMM_S;
    this->memory_write(this->gpr[rs1] + imm, this->gpr[rs2], 4);
}

void RV32Core::do_beq() {
    RS1; RS2; IMM_B;
    if (this->gpr[rs1] == this->gpr[rs2]) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_bge() {
    RS1; RS2; IMM_B;
    if ((sword_t)this->gpr[rs1] >= (sword_t)this->gpr[rs2]) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_bgeu() {
    RS1; RS2; IMM_B;
    if (this->gpr[rs1] >= this->gpr[rs2]) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_blt() {
    RS1; RS2; IMM_B;
    if ((sword_t)this->gpr[rs1] < (sword_t)this->gpr[rs2]) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_bltu() {
    RS1; RS2; IMM_B;
    if (this->gpr[rs1] < this->gpr[rs2]) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_bne() {
    RS1; RS2; IMM_B;
    if (this->gpr[rs1] != this->gpr[rs2]) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_jal() {
    RD; IMM_J;
    this->set_gpr(rd, this->pc + 4);
    this->npc = this->pc + imm;
}

void RV32Core::do_jalr() {
    RD; RS1; IMM_I;
    this->set_gpr(rd, this->pc + 4);
    this->npc = this->gpr[rs1] + imm;
}

void RV32Core::do_lui() {
    RD; IMM_U;
    this->set_gpr(rd, imm);
}

void RV32Core::do_auipc() {
    RD; IMM_U;
    this->set_gpr(rd, this->pc + imm);
}
