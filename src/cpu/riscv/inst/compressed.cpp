#include "cpu/riscv/core.h"
#include "./local-decoder.h"
#include "cpu/word.h"
#include "log.h"
#include <cstdint>

#define REQUIRE(cond) do {if (!(cond)) { this->do_invalid_inst(); return; }} while(0);
#define REQUIRE_NOT_ZERO(v) REQUIRE(v != 0)

using namespace kxemu::cpu;

void RVCore::do_c_lwsp() {
    int rd = BITS(11, 7);
    word_t imm = BITS(12, 12) << 5 | BITS(3, 2) << 6 | BITS(6, 4) << 2;

    REQUIRE_NOT_ZERO(rd);

    sword_t data = (int32_t)this->memory_load(this->gpr[2] + imm, 4);
    this->set_gpr(rd, data);
}

void RVCore::do_c_swsp() {
    int rs2 = BITS(6, 2);
    word_t imm = BITS(8, 7) << 6 | BITS(12, 9) << 2;
    this->memory_store(this->gpr[2] + imm, this->gpr[rs2], 4);
}

void RVCore::do_c_lw() {
    int rd  = BITS(4, 2) + 8;
    int rs1 = BITS(9, 7) + 8;
    word_t imm = BITS(5, 5) << 6 | BITS(12, 10) << 3 | BITS(6, 6) << 2;
    sword_t data = (int32_t)this->memory_load(this->gpr[rs1] + imm, 4);
    this->set_gpr(rd, data);
}

void RVCore::do_c_sw() {
    int rs1 = BITS(9, 7) + 8;
    int rs2 = BITS(4, 2) + 8;
    word_t imm = BITS(5, 5) << 6 | BITS(12, 10) << 3 | BITS(6, 6) << 2;
    this->memory_store(this->gpr[rs1] + imm, this->gpr[rs2], 4);
}

void RVCore::do_c_j() {
    sword_t imm = SEXT(
        BITS(12, 12) << 11 |
        BITS( 8,  8) << 10 |
        BITS(10,  9) <<  8 |
        BITS( 7,  7) <<  6 |
        BITS( 6,  6) <<  7 |
        BITS( 2,  2) <<  5 |
        BITS(11, 11) <<  4 |
        BITS( 5,  3) <<  1,
        12);
    this->npc = this->pc + imm;
}

void RVCore::do_c_jal() {
    sword_t imm = SEXT(
        BITS(12, 12) << 11 |
        BITS( 8,  8) << 10 |
        BITS(10,  9) <<  8 |
        BITS( 6,  6) <<  7 |
        BITS( 7,  7) <<  6 |
        BITS( 2,  2) <<  5 |
        BITS(11, 11) <<  4 |
        BITS( 5,  3) <<  1,
        12);
    this->set_gpr(1, this->pc + 2);
    this->npc = this->pc + imm;
}

void RVCore::do_c_jr() {
    int rs1 = BITS(11, 7);

    REQUIRE_NOT_ZERO(rs1);

    this->npc = this->gpr[rs1];
}

void RVCore::do_c_jalr() {
    int rs1 = BITS(11, 7);
    
    REQUIRE_NOT_ZERO(rs1);
    
    this->set_gpr(1, this->pc + 2);
    this->npc = this->gpr[rs1];
}

void RVCore::do_c_beqz() {
    int rs1 = BITS(9, 7) + 8;
    sword_t imm = SEXT(
        BITS(12, 12) << 8 |
        BITS( 6,  5) << 6 |
        BITS( 2,  2) << 5 |
        BITS(11, 10) << 3 |
        BITS( 4,  3) << 1,
        9);
    if (this->gpr[rs1] == 0) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_c_bnez() {
    int rs1 = BITS(9, 7) + 8;
    sword_t imm = SEXT(
        BITS(12, 12) << 8 |
        BITS( 6,  5) << 6 |
        BITS( 2,  2) << 5 |
        BITS(11, 10) << 3 |
        BITS( 4,  3) << 1,
        9);
    if (this->gpr[rs1] != 0) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_c_li() {
    int rd = BITS(11, 7);
    sword_t imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);
    
    REQUIRE_NOT_ZERO(rd);
    
    this->set_gpr(rd, imm);
}

void RVCore::do_c_lui() {
    int rd = BITS(11, 7);
    sword_t imm = SEXT(BITS(12, 12) << 17 | BITS(6, 2) << 12, 18);
    
    REQUIRE_NOT_ZERO(rd);
    REQUIRE_NOT_ZERO(imm);

    this->set_gpr(rd, imm);
}

void RVCore::do_c_addi() {
    int rd = BITS(11, 7);
    sword_t imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);

    REQUIRE_NOT_ZERO(imm);

    this->set_gpr(rd, this->gpr[rd] + imm);
}

void RVCore::do_c_addi16sp() {;
    sword_t imm = SEXT(
        BITS(12, 12) << 9 |
        BITS( 4,  3) << 7 |
        BITS( 5,  5) << 6 |
        BITS( 2,  2) << 5 |
        BITS( 6,  6) << 4,
        10);

    REQUIRE_NOT_ZERO(imm);

    this->set_gpr(2, this->gpr[2] + imm);
}

void RVCore::do_c_addi4spn() {
    int rd = BITS(4, 2) + 8;
    sword_t imm = BITS(10, 7) << 6 | BITS(12, 11) << 4 | BITS(5, 5) << 3 | BITS(6, 6) << 2;
    
    REQUIRE_NOT_ZERO(imm);

    this->set_gpr(rd, this->gpr[2] + imm);
}

void RVCore::do_c_slli() {
    int rd = BITS(11, 7);
    int shamt = BITS(12, 12) << 5 | BITS(6, 2);

    REQUIRE_NOT_ZERO(rd);

    this->set_gpr(rd, this->gpr[rd] << shamt);
}

void RVCore::do_c_srli() {
    int rd = BITS(9, 7) + 8;
    int shamt = BITS(12, 12) << 5 | BITS(6, 2);
    this->set_gpr(rd, this->gpr[rd] >> shamt);
}

void RVCore::do_c_srai() {
    int rd = BITS(9, 7) + 8;
    int shamt = BITS(12, 12) << 5 | BITS(6, 2);
    this->set_gpr(rd, (sword_t)this->gpr[rd] >> shamt);
}

void RVCore::do_c_andi() {
    int rd = BITS(9, 7) + 8;
    sword_t imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);
    this->set_gpr(rd, this->gpr[rd] & imm);
}

void RVCore::do_c_mv() {
    int rd  = BITS(11, 7);
    int rs2 = BITS( 6, 2);

    REQUIRE_NOT_ZERO(rd);

    this->set_gpr(rd, this->gpr[rs2]);
}

void RVCore::do_c_add() {
    int rd  = BITS(11, 7);
    int rs2 = BITS( 6, 2);

    REQUIRE_NOT_ZERO(rd);

    this->set_gpr(rd, this->gpr[rd] + this->gpr[rs2]);
}

void RVCore::do_c_sub() {
    int rd  = BITS(9, 7) + 8;
    int rs2 = BITS(4, 2) + 8;
    this->set_gpr(rd, this->gpr[rd] - this->gpr[rs2]);
}

void RVCore::do_c_xor() {
    int rd  = BITS(9, 7) + 8;
    int rs2 = BITS(4, 2) + 8;
    this->set_gpr(rd, this->gpr[rd] ^ this->gpr[rs2]);
}

void RVCore::do_c_or() {
    int rd  = BITS(9, 7) + 8;
    int rs2 = BITS(4, 2) + 8;
    this->set_gpr(rd, this->gpr[rd] | this->gpr[rs2]);
}

void RVCore::do_c_and() {
    int rd  = BITS(9, 7) + 8;
    int rs2 = BITS(4, 2) + 8;
    this->set_gpr(rd, this->gpr[rd] & this->gpr[rs2]);
}

void RVCore::do_c_nop() {
    // Do nothing
}

#ifdef KXEMU_ISA64

void RVCore::do_c_ldsp() {
    int rd = BITS(11, 7);
    word_t imm = BITS(12, 12) << 5 | BITS(4, 2) << 6 | BITS(6, 5) << 3;

    REQUIRE_NOT_ZERO(rd);

    this->set_gpr(rd, this->memory_load(this->gpr[2] + imm, 8));
}

void RVCore::do_c_sdsp() {
    int rs2 = BITS(6, 2);
    word_t imm = BITS(9, 7) << 6 | BITS(12, 10) << 3;
    this->memory_store(this->gpr[2] + imm, this->gpr[rs2], 8);
}

void RVCore::do_c_ld() {
    int rd  = BITS(4, 2) + 8;
    int rs1 = BITS(9, 7) + 8;
    word_t imm = BITS(6, 5) << 6 | BITS(12, 10) << 3;
    this->set_gpr(rd, this->memory_load(this->gpr[rs1] + imm, 8));
}

void RVCore::do_c_sd() {
    int rs1 = BITS(9, 7) + 8;
    int rs2 = BITS(4, 2) + 8;
    word_t imm = BITS(6, 5) << 6 | BITS(12, 10) << 3;
    this->memory_store(this->gpr[rs1] + imm, this->gpr[rs2], 8);
}

void RVCore::do_c_addiw() {
    int rd = BITS(11, 7);
    int32_t imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);

    REQUIRE_NOT_ZERO(rd);

    sword_t result = (int32_t)this->gpr[rd] + imm;
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_c_addw() {
    int rd  = BITS(9, 7) + 8;
    int rs2 = BITS(4, 2) + 8;
    sword_t result = (int32_t)this->gpr[rd] + (int32_t)this->gpr[rs2];
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_c_subw() {
    int rd  = BITS(9, 7) + 8;
    int rs2 = BITS(4, 2) + 8;
    sword_t result = (int32_t)this->gpr[rd] - (int32_t)this->gpr[rs2];
    this->set_gpr(rd, (word_t)result);
}

#endif
