#include "cpu/riscv/core.h"
#include "./local-decoder.h"
#include "cpu/word.h"
#include "log.h"
#include <cstdint>

#define REQUIRE(cond) do {if (!(cond)) { this->do_invalid_inst(); return; }} while(0);
#define REQUIRE_NOT_ZERO(v) REQUIRE(v != 0)

#define RD  unsigned int rd  = decodeInfo.rd
#define RS1 unsigned int rs1 = decodeInfo.rs1
#define RS2 unsigned int rs2 = decodeInfo.rs2
#define IMM word_t imm = decodeInfo.imm

using namespace kxemu::cpu;

void RVCore::do_c_lwsp(const DecodeInfo &decodeInfo) {
    // int rd = BITS(11, 7);
    // word_t imm = BITS(12, 12) << 5 | BITS(3, 2) << 6 | BITS(6, 4) << 2;

    RD; IMM;

    REQUIRE_NOT_ZERO(rd);

    sword_t data = (int32_t)this->memory_load(this->gpr[2] + imm, 4);
    this->set_gpr(rd, data);
}

void RVCore::do_c_swsp(const DecodeInfo &decodeInfo) {
    // int rs2 = BITS(6, 2);
    // word_t imm = BITS(8, 7) << 6 | BITS(12, 9) << 2;

    RS2; IMM;

    this->memory_store(this->gpr[2] + imm, this->gpr[rs2], 4);
}

void RVCore::do_c_lw(const DecodeInfo &decodeInfo) {
    // int rd  = BITS(4, 2) + 8;
    // int rs1 = BITS(9, 7) + 8;
    // word_t imm = BITS(5, 5) << 6 | BITS(12, 10) << 3 | BITS(6, 6) << 2;

    RD; RS1; IMM;

    sword_t data = (int32_t)this->memory_load(this->gpr[rs1] + imm, 4);
    this->set_gpr(rd, data);
}

void RVCore::do_c_sw(const DecodeInfo &decodeInfo) {
    // int rs1 = BITS(9, 7) + 8;
    // int rs2 = BITS(4, 2) + 8;
    // word_t imm = BITS(5, 5) << 6 | BITS(12, 10) << 3 | BITS(6, 6) << 2;

    RS1; RS2; IMM;

    this->memory_store(this->gpr[rs1] + imm, this->gpr[rs2], 4);
}

void RVCore::do_c_j(const DecodeInfo &decodeInfo) {
    // sword_t imm = SEXT(
    //     BITS(12, 12) << 11 |
    //     BITS( 8,  8) << 10 |
    //     BITS(10,  9) <<  8 |
    //     BITS( 7,  7) <<  6 |
    //     BITS( 6,  6) <<  7 |
    //     BITS( 2,  2) <<  5 |
    //     BITS(11, 11) <<  4 |
    //     BITS( 5,  3) <<  1,
    //     12);

    IMM;

    this->npc = this->pc + imm;
}

void RVCore::do_c_jal(const DecodeInfo &decodeInfo) {
    // sword_t imm = SEXT(
    //     BITS(12, 12) << 11 |
    //     BITS( 8,  8) << 10 |
    //     BITS(10,  9) <<  8 |
    //     BITS( 6,  6) <<  7 |
    //     BITS( 7,  7) <<  6 |
    //     BITS( 2,  2) <<  5 |
    //     BITS(11, 11) <<  4 |
    //     BITS( 5,  3) <<  1,
    //     12);

    IMM;

    this->set_gpr(1, this->pc + 2);
    this->npc = this->pc + imm;
}

void RVCore::do_c_jr(const DecodeInfo &decodeInfo) {
    // int rs1 = BITS(11, 7);

    RS1;

    REQUIRE_NOT_ZERO(rs1);

    this->npc = this->gpr[rs1];
}

void RVCore::do_c_jalr(const DecodeInfo &decodeInfo) {
    // int rs1 = BITS(11, 7);
    RS1;
    
    REQUIRE_NOT_ZERO(rs1);
    
    this->set_gpr(1, this->pc + 2);
    this->npc = this->gpr[rs1];
}

void RVCore::do_c_beqz(const DecodeInfo &decodeInfo) {
    // int rs1 = BITS(9, 7) + 8;
    // sword_t imm = SEXT(
    //     BITS(12, 12) << 8 |
    //     BITS( 6,  5) << 6 |
    //     BITS( 2,  2) << 5 |
    //     BITS(11, 10) << 3 |
    //     BITS( 4,  3) << 1,
    //     9);

    RS1; IMM;
    
    if (this->gpr[rs1] == 0) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_c_bnez(const DecodeInfo &decodeInfo) {
    // int rs1 = BITS(9, 7) + 8;
    // sword_t imm = SEXT(
    //     BITS(12, 12) << 8 |
    //     BITS( 6,  5) << 6 |
    //     BITS( 2,  2) << 5 |
    //     BITS(11, 10) << 3 |
    //     BITS( 4,  3) << 1,
    //     9);

    RS1; IMM;

    if (this->gpr[rs1] != 0) {
        this->npc = this->pc + imm;
    }
}

void RVCore::do_c_li(const DecodeInfo &decodeInfo) {
    // int rd = BITS(11, 7);
    // sword_t imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);

    RD; IMM;
    
    REQUIRE_NOT_ZERO(rd);
    
    this->set_gpr(rd, imm);
}

void RVCore::do_c_lui(const DecodeInfo &decodeInfo) {
    // int rd = BITS(11, 7);
    // sword_t imm = SEXT(BITS(12, 12) << 17 | BITS(6, 2) << 12, 18);

    RD; IMM;
    
    REQUIRE_NOT_ZERO(rd);
    REQUIRE_NOT_ZERO(imm);

    this->set_gpr(rd, imm);
}

void RVCore::do_c_addi(const DecodeInfo &decodeInfo) {
    // int rd = BITS(11, 7);
    // sword_t imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);

    RD; IMM;

    REQUIRE_NOT_ZERO(imm);

    this->set_gpr(rd, this->gpr[rd] + imm);
}

void RVCore::do_c_addi16sp(const DecodeInfo &decodeInfo) {;
    // sword_t imm = SEXT(
    //     BITS(12, 12) << 9 |
    //     BITS( 4,  3) << 7 |
    //     BITS( 5,  5) << 6 |
    //     BITS( 2,  2) << 5 |
    //     BITS( 6,  6) << 4,
    //     10);

    IMM;

    REQUIRE_NOT_ZERO(imm);

    this->set_gpr(2, this->gpr[2] + imm);
}

void RVCore::do_c_addi4spn(const DecodeInfo &decodeInfo) {
    // int rd = BITS(4, 2) + 8;
    // sword_t imm = BITS(10, 7) << 6 | BITS(12, 11) << 4 | BITS(5, 5) << 3 | BITS(6, 6) << 2;

    RD; IMM;
    
    REQUIRE_NOT_ZERO(imm);

    this->set_gpr(rd, this->gpr[2] + imm);
}

void RVCore::do_c_slli(const DecodeInfo &decodeInfo) {
    // int rd = BITS(11, 7);
    // int shamt = BITS(12, 12) << 5 | BITS(6, 2);

    RD; IMM;
    unsigned int shamt = imm;

    REQUIRE_NOT_ZERO(rd);

    this->set_gpr(rd, this->gpr[rd] << shamt);
}

void RVCore::do_c_srli(const DecodeInfo &decodeInfo) {
    // int rd = BITS(9, 7) + 8;
    // int shamt = BITS(12, 12) << 5 | BITS(6, 2);

    RD; IMM;

    this->set_gpr(rd, this->gpr[rd] >> imm);
}

void RVCore::do_c_srai(const DecodeInfo &decodeInfo) {
    // int rd = BITS(9, 7) + 8;
    // int shamt = BITS(12, 12) << 5 | BITS(6, 2);

    RD; IMM;

    this->set_gpr(rd, (sword_t)this->gpr[rd] >> imm);
}

void RVCore::do_c_andi(const DecodeInfo &decodeInfo) {
    // int rd = BITS(9, 7) + 8;
    // sword_t imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);

    RD; IMM;

    this->set_gpr(rd, this->gpr[rd] & imm);
}

void RVCore::do_c_mv(const DecodeInfo &decodeInfo) {
    // int rd  = BITS(11, 7);
    // int rs2 = BITS( 6, 2);

    RD; RS2;

    REQUIRE_NOT_ZERO(rd);

    this->set_gpr(rd, this->gpr[rs2]);
}

void RVCore::do_c_add(const DecodeInfo &decodeInfo) {
    // int rd  = BITS(11, 7);
    // int rs2 = BITS( 6, 2);

    RD; RS2;

    REQUIRE_NOT_ZERO(rd);

    this->set_gpr(rd, this->gpr[rd] + this->gpr[rs2]);
}

void RVCore::do_c_sub(const DecodeInfo &decodeInfo) {
    // int rd  = BITS(9, 7) + 8;
    // int rs2 = BITS(4, 2) + 8;

    RD; RS2;

    this->set_gpr(rd, this->gpr[rd] - this->gpr[rs2]);
}

void RVCore::do_c_xor(const DecodeInfo &decodeInfo) {
    // int rd  = BITS(9, 7) + 8;
    // int rs2 = BITS(4, 2) + 8;

    RD; RS2;

    this->set_gpr(rd, this->gpr[rd] ^ this->gpr[rs2]);
}

void RVCore::do_c_or(const DecodeInfo &decodeInfo) {
    // int rd  = BITS(9, 7) + 8;
    // int rs2 = BITS(4, 2) + 8;

    RD; RS2;

    this->set_gpr(rd, this->gpr[rd] | this->gpr[rs2]);
}

void RVCore::do_c_and(const DecodeInfo &decodeInfo) {
    // int rd  = BITS(9, 7) + 8;
    // int rs2 = BITS(4, 2) + 8;

    RD; RS2;

    this->set_gpr(rd, this->gpr[rd] & this->gpr[rs2]);
}

void RVCore::do_c_nop(const DecodeInfo &) {
    // Do nothing
}

#ifdef KXEMU_ISA64

void RVCore::do_c_ldsp(const DecodeInfo &decodeInfo) {
    // int rd = BITS(11, 7);
    // word_t imm = BITS(12, 12) << 5 | BITS(4, 2) << 6 | BITS(6, 5) << 3;

    RD; IMM;

    REQUIRE_NOT_ZERO(rd);

    this->set_gpr(rd, this->memory_load(this->gpr[2] + imm, 8));
}

void RVCore::do_c_sdsp(const DecodeInfo &decodeInfo) {
    // int rs2 = BITS(6, 2);
    // word_t imm = BITS(9, 7) << 6 | BITS(12, 10) << 3;

    RS2; IMM;

    this->memory_store(this->gpr[2] + imm, this->gpr[rs2], 8);
}

void RVCore::do_c_ld(const DecodeInfo &decodeInfo) {
    // int rd  = BITS(4, 2) + 8;
    // int rs1 = BITS(9, 7) + 8;
    // word_t imm = BITS(6, 5) << 6 | BITS(12, 10) << 3;

    RD; RS1; IMM;

    this->set_gpr(rd, this->memory_load(this->gpr[rs1] + imm, 8));
}

void RVCore::do_c_sd(const DecodeInfo &decodeInfo) {
    // int rs1 = BITS(9, 7) + 8;
    // int rs2 = BITS(4, 2) + 8;
    // word_t imm = BITS(6, 5) << 6 | BITS(12, 10) << 3;

    RS1; RS2; IMM;

    this->memory_store(this->gpr[rs1] + imm, this->gpr[rs2], 8);
}

void RVCore::do_c_addiw(const DecodeInfo &decodeInfo) {
    // int rd = BITS(11, 7);
    // int32_t imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);

    RD; IMM;

    REQUIRE_NOT_ZERO(rd);

    sword_t result = (int32_t)this->gpr[rd] + imm;
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_c_addw(const DecodeInfo &decodeInfo) {
    // int rd  = BITS(9, 7) + 8;
    // int rs2 = BITS(4, 2) + 8;

    RD; RS2;
    
    sword_t result = (int32_t)this->gpr[rd] + (int32_t)this->gpr[rs2];
    this->set_gpr(rd, (word_t)result);
}

void RVCore::do_c_subw(const DecodeInfo &decodeInfo) {
    // int rd  = BITS(9, 7) + 8;
    // int rs2 = BITS(4, 2) + 8;

    RD; RS2;

    sword_t result = (int32_t)this->gpr[rd] - (int32_t)this->gpr[rs2];
    this->set_gpr(rd, (word_t)result);
}

#endif
