#include "cpu/decoder.h"
#include "isa/riscv32/core.h"
#include <cstdint>

#define INSTPAT(pat, name) this->cdecoder.add(pat, &RV32Core::do_##name)
#define BITS(hi, lo) sub_bits(this->inst, hi, lo)

static inline uint32_t sub_bits(uint64_t bits, int hi, int lo) {
    return (bits >> lo) & ((1 << (hi - lo + 1)) - 1);
}

static inline int32_t sign_extend(uint32_t bits, int from) {
    int shift = 32 - from;
    return (int32_t)(bits << shift) >> shift;
}

// NOTE: Pay attention to the order of the instructions in the init_c_decoder function
void RV32Core::init_c_decoder() {
    this->cdecoder.init(this);

    INSTPAT("100 1 00000 00000 10", ebreak); // same as do_ebreak

    INSTPAT("010 ? ????? ????? 10", c_lwsp);
    INSTPAT("110 ? ????? ????? 10", c_swsp);
    
    INSTPAT("010 ??? ??? ?? ??? 00", c_lw);
    INSTPAT("110 ??? ??? ?? ??? 00", c_sw);

    INSTPAT("101 ??????????? 01", c_j);
    INSTPAT("001 ??????????? 01", c_jal);
    INSTPAT("100 0 ????? 00000 10", c_jr);
    INSTPAT("100 1 ????? 00000 10", c_jalr);

    INSTPAT("110 ??? ??? ?? ??? 01", c_beqz);
    INSTPAT("111 ??? ??? ?? ??? 01", c_bnez);

    INSTPAT("010 ? ????? ????? 01", c_li);
    INSTPAT("011 ? ????? ????? 01", c_lui);

    INSTPAT("000 ? ????? ????? 01", c_addi);
    INSTPAT("011 ? 00010 ????? 01", c_addi16sp);
    INSTPAT("000 ???????? ??? 01", c_addi4spn);

    INSTPAT("000 ? ????? ????? 10", c_slli);
    INSTPAT("100 ? 00 ??? ????? 10", c_srli);
    INSTPAT("100 ? 01 ??? ????? 10", c_srai);
    INSTPAT("100 ? 10 ??? ????? 10", c_andi);

    INSTPAT("100 0 ????? ????? 10", c_mv);
    INSTPAT("100 1 ????? ????? 10", c_add);

    INSTPAT("100 1 11 ??? 00 ??? 01", c_sub);
    INSTPAT("100 0 11 ??? 01 ??? 01", c_xor);
    INSTPAT("100 0 11 ??? 10 ??? 01", c_or);
    INSTPAT("100 0 11 ??? 11 ??? 01", c_and);
}

void RV32Core::do_c_lwsp() {
    int rd = sub_bits(this->inst, 9, 7) + 8;
    int32_t imm = sign_extend(
        sub_bits(this->inst, 5, 5) << 6 | 
        sub_bits(this->inst, 12, 10) << 3 | 
        sub_bits(this->inst, 6, 6) << 2 | 
        sub_bits(this->inst, 4, 4) << 1, 
        7);
    this->set_gpr(rd, this->memory->read(this->gpr[2] + imm, 4));
}

void RV32Core::do_c_swsp() {
    int rs2 = sub_bits(this->inst, 6, 2);
    int32_t imm = sign_extend(
        sub_bits(this->inst, 9, 8) << 6 |
        sub_bits(this->inst, 12, 10) << 2,
        24);
    this->memory->write(this->gpr[2] + imm, 4, this->gpr[rs2]);
}

void RV32Core::do_c_lw() {
    int rd  = sub_bits(this->inst, 9, 7) + 8;
    int rs1 = sub_bits(this->inst, 4, 2) + 8;
    int32_t imm = sign_extend(
        BITS(5, 5) << 6 |
        BITS(12, 10) << 3 |
        BITS(6, 6) << 2,
        7);
    this->set_gpr(rd, this->memory->read(this->gpr[rs1] + imm, 4));
}

void RV32Core::do_c_sw() {
    int rs1 = sub_bits(this->inst, 9, 7) + 8;
    int rs2 = sub_bits(this->inst, 4, 2) + 8;
    int32_t imm = sign_extend(
        BITS(5, 5) << 6 |
        BITS(12, 10) << 3 |
        BITS(6, 6) << 2,
        7);
    this->memory->write(this->gpr[rs1] + imm, 4, this->gpr[rs2]);
}

void RV32Core::do_c_j() {
    int32_t imm = sign_extend(
        BITS(12, 12) << 11 |
        BITS(8, 8) << 10 |
        BITS(10, 9) << 8 |
        BITS(7, 7) << 7 |
        BITS(6, 6) << 6 |
        BITS(2, 2) << 5 |
        BITS(11, 11) << 4 |
        BITS(5, 3) << 1,
        12);
    this->npc = this->pc + imm;
}

void RV32Core::do_c_jal() {
    int rd = sub_bits(this->inst, 11, 7) + 8;
    int32_t imm = sign_extend(
        BITS(12, 12) << 11 |
        BITS(8, 8) << 10 |
        BITS(10, 9) << 8 |
        BITS(7, 7) << 7 |
        BITS(6, 6) << 6 |
        BITS(2, 2) << 5 |
        BITS(11, 11) << 4 |
        BITS(5, 3) << 1,
        12);
    this->set_gpr(rd, this->pc + 2);
    this->npc = this->pc + imm;
}

void RV32Core::do_c_jr() {
    int rs1 = sub_bits(this->inst, 11, 7);
    this->npc = this->gpr[rs1];
}

void RV32Core::do_c_jalr() {
    int rs1 = sub_bits(this->inst, 11, 7);
    this->set_gpr(1, this->pc + 2);
    this->npc = this->gpr[rs1];
}

void RV32Core::do_c_beqz() {
    int rs1 = sub_bits(this->inst, 9, 7) + 8;
    int32_t imm = sign_extend(
        BITS(12, 12) << 8 |
        BITS(6, 5) << 6 |
        BITS(2, 2) << 5 |
        BITS(11, 10) << 3 |
        BITS(4, 3) << 1,
        9);
    if (this->gpr[rs1] == 0) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_c_bnez() {
    int rs1 = sub_bits(this->inst, 9, 7) + 8;
    int32_t imm = sign_extend(
        BITS(12, 12) << 8 |
        BITS(6, 5) << 6 |
        BITS(2, 2) << 5 |
        BITS(11, 10) << 3 |
        BITS(4, 3) << 1,
        9);
    if (this->gpr[rs1] != 0) {
        this->npc = this->pc + imm;
    }
}

void RV32Core::do_c_li() {
    int rd = sub_bits(this->inst, 11, 7);
    int32_t imm = sign_extend(
        BITS(11, 11) << 5 |
        BITS(6, 2),
        6);
    this->set_gpr(rd, imm);
}

void RV32Core::do_c_lui() {
    int rd = sub_bits(this->inst, 11, 7);
    uint32_t imm = BITS(12, 12) << 17 | BITS(6, 2) << 12;
    this->set_gpr(rd, imm);
}

void RV32Core::do_c_addi() {
    int rd = sub_bits(this->inst, 11, 7);
    int32_t imm = sign_extend(
        BITS(12, 12) << 5 |
        BITS(6, 2),
        6);
    this->set_gpr(rd, this->gpr[rd] + imm);
}

void RV32Core::do_c_addi16sp() {
    int rd = 2 + sub_bits(this->inst, 9, 7);
    int32_t imm = sign_extend(
        BITS(12, 12) << 9 |
        BITS(4, 3) << 7 |
        BITS(5, 5) << 6 |
        BITS(2, 2) << 5 |
        BITS(6, 6) << 4,
        10);
    this->set_gpr(rd, this->gpr[2] + imm);
}

void RV32Core::do_c_addi4spn() {
    int rd = sub_bits(this->inst, 4, 2) + 8;
    int32_t imm = sign_extend(
        BITS(10, 7) << 6 |
        BITS(12, 11) << 4 |
        BITS(5, 5) << 3 |
        BITS(6, 6) << 2,
        10);
    this->set_gpr(rd, this->gpr[2] + imm);
}

void RV32Core::do_c_slli() {
    int rd = BITS(11, 7);
    int shamt = BITS(6, 2);
    this->set_gpr(rd, this->gpr[rd] << shamt);
}

void RV32Core::do_c_srli() {
    int rd = BITS(9, 7) + 8;
    int shamt = BITS(6, 2);
    this->set_gpr(rd, this->gpr[rd] >> shamt);
}

void RV32Core::do_c_srai() {
    int rd = BITS(9, 7) + 8;
    int shamt = BITS(6, 2);
    this->set_gpr(rd, (int32_t)this->gpr[rd] >> shamt);
}

void RV32Core::do_c_andi() {
    int rd = BITS(9, 7) + 8;
    int32_t imm = sign_extend(BITS(12, 12) << 5 | BITS(6, 2), 6);
    this->set_gpr(rd, this->gpr[rd] & imm);
}

void RV32Core::do_c_mv() {
    int rd = BITS(11, 7);
    int rs2 = BITS(6, 2);
    this->set_gpr(rd, this->gpr[rs2]);
}

void RV32Core::do_c_add() {
    int rd = BITS(11, 7);
    int rs2 = BITS(6, 2);
    this->set_gpr(rd, this->gpr[rd] + this->gpr[rs2]);
}

void RV32Core::do_c_sub() {
    int rd = BITS(9, 7) + 8;
    int rs2 = BITS(4, 2) + 8;
    this->set_gpr(rd, this->gpr[rd] - this->gpr[rs2]);
}

void RV32Core::do_c_xor() {
    int rd = BITS(9, 7) + 8;
    int rs2 = BITS(4, 2) + 8;
    this->set_gpr(rd, this->gpr[rd] ^ this->gpr[rs2]);
}

void RV32Core::do_c_or() {
    int rd = BITS(9, 7) + 8;
    int rs2 = BITS(4, 2) + 8;
    this->set_gpr(rd, this->gpr[rd] | this->gpr[rs2]);
}

void RV32Core::do_c_and() {
    int rd = BITS(9, 7) + 8;
    int rs2 = BITS(4, 2) + 8;
    this->set_gpr(rd, this->gpr[rd] & this->gpr[rs2]);
}
