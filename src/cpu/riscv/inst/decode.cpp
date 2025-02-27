#include "cpu/riscv/core.h"
#include "cpu/word.h"
#include <cstdint>

using namespace kxemu::cpu;

#define BITS(hi, lo) ((word_t)((this->inst >> lo) & ((1 << (hi - lo + 1)) - 1))) // Extract bits from hi to lo
#define SEXT(bits, from) (sword_t)((int32_t)((bits) << (32 - (from))) >> (32 - (from))) // Signed extend

void RVCore::decode_r() {
    this->gDecodeInfo.rd  = BITS(11,  7);
    this->gDecodeInfo.rs1 = BITS(19, 15);
    this->gDecodeInfo.rs2 = BITS(24, 20);
}

void RVCore::decode_i() {
    this->gDecodeInfo.rd  = BITS(11,  7);
    this->gDecodeInfo.rs1 = BITS(19, 15);
    this->gDecodeInfo.imm = SEXT(BITS(31, 20), 12);
}

void RVCore::decode_shifti() {
    this->gDecodeInfo.rd  = BITS(11,  7);
    this->gDecodeInfo.rs1 = BITS(19, 15);
#ifdef KXEMU_ISA64
    this->gDecodeInfo.imm = BITS(25, 20);
#else
    this->gDecodeInfo.imm = BITS(24, 20);
#endif
}

void RVCore::decode_s() {
    this->gDecodeInfo.rs1 = BITS(19, 15);
    this->gDecodeInfo.rs2 = BITS(24, 20);
    this->gDecodeInfo.imm = SEXT((BITS(31, 25) << 5) | BITS(11, 7), 12);
}

void RVCore::decode_b() {
    this->gDecodeInfo.rs1 = BITS(19, 15);
    this->gDecodeInfo.rs2 = BITS(24, 20);
    this->gDecodeInfo.imm = SEXT((BITS(31, 31) << 12) | (BITS(30, 25) << 5) | (BITS(11, 8) << 1) | (BITS(7, 7) << 11), 13);
}

void RVCore::decode_j() {
    this->gDecodeInfo.rd  = BITS(11,  7);
    this->gDecodeInfo.imm = SEXT((BITS(31, 31) << 20) | (BITS(30, 21) << 1) | (BITS(20, 20) << 11) | (BITS(19, 12) << 12), 21);
}

void RVCore::decode_u() {
    this->gDecodeInfo.rd  = BITS(11, 7);
    this->gDecodeInfo.imm = SEXT(BITS(31, 12) << 12, 32);
}

void RVCore::decode_csrr() {
    this->gDecodeInfo.rd  = BITS(11,  7);
    this->gDecodeInfo.rs1 = BITS(19, 15);
    this->gDecodeInfo.csr = BITS(31, 20);
}

void RVCore::decode_csri() {
    this->gDecodeInfo.rd  = BITS(11,  7);
    this->gDecodeInfo.csr = BITS(31, 20);
    this->gDecodeInfo.imm = BITS(19, 15);
}

void RVCore::decode_c_lwsp() {
    this->gDecodeInfo.rd  = BITS(11, 7);
    this->gDecodeInfo.imm = BITS(12, 12) << 5 | BITS(3, 2) << 6 | BITS(6, 4) << 2;
}

void RVCore::decode_c_swsp() {
    this->gDecodeInfo.rs1 = 2;
    this->gDecodeInfo.rs2 = BITS(6, 2);
    this->gDecodeInfo.imm = BITS(8, 7) << 6 | BITS(12, 9) << 2;
}

void RVCore::decode_c_lwsw() {
    this->gDecodeInfo.rd  = BITS(4, 2) + 8;
    this->gDecodeInfo.rs1 = BITS(9, 7) + 8;
    this->gDecodeInfo.rs2 = BITS(4, 2) + 8;
    this->gDecodeInfo.imm = BITS(5, 5) << 6 | BITS(12, 10) << 3 | BITS(6, 6) << 2;
}

void RVCore::decode_c_j() {
    this->gDecodeInfo.rs1 = BITS(11, 7);
    this->gDecodeInfo.imm = SEXT(
        BITS(12, 12) << 11 |
        BITS( 8,  8) << 10 |
        BITS(10,  9) <<  8 |
        BITS( 7,  7) <<  6 |
        BITS( 6,  6) <<  7 |
        BITS( 2,  2) <<  5 |
        BITS(11, 11) <<  4 |
        BITS( 5,  3) <<  1,
        12);
}

void RVCore::decode_c_b() {
    this->gDecodeInfo.rs1 = BITS(9, 7) + 8;
    this->gDecodeInfo.imm = SEXT(
        BITS(12, 12) << 8 |
        BITS( 6,  5) << 6 |
        BITS( 2,  2) << 5 |
        BITS(11, 10) << 3 |
        BITS( 4,  3) << 1,
        9);
}

void RVCore::decode_c_li() {
    this->gDecodeInfo.rd  = BITS(11, 7);
    this->gDecodeInfo.imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);
}

void RVCore::decode_c_lui() {
    this->gDecodeInfo.rd  = BITS(11, 7);
    this->gDecodeInfo.imm = SEXT(BITS(12, 12) << 17 | BITS(6, 2) << 12, 18);
}

void RVCore::decode_c_addi() {
    this->gDecodeInfo.rd  = BITS(11, 7);
    this->gDecodeInfo.imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);
}

void RVCore::decode_c_addi16sp() {
    this->gDecodeInfo.imm = SEXT(
        BITS(12, 12) << 9 |
        BITS( 4,  3) << 7 |
        BITS( 5,  5) << 6 |
        BITS( 2,  2) << 5 |
        BITS( 6,  6) << 4,
        10);
}

void RVCore::decode_c_addi4spn() {
    this->gDecodeInfo.rd  = BITS(4, 2) + 8;
    this->gDecodeInfo.imm = BITS(10, 7) << 6 | BITS(12, 11) << 4 | BITS(5, 5) << 3 | BITS(6, 6) << 2;
}

void RVCore::decode_c_slli() {
    this->gDecodeInfo.rd  = BITS(11, 7);
    this->gDecodeInfo.imm = BITS(12, 12) << 5 | BITS(6, 2);
}

void RVCore::decode_c_i() {
    this->gDecodeInfo.rd  = BITS(9, 7) + 8;
    this->gDecodeInfo.imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);
}

void RVCore::decode_c_shifti() {
    this->gDecodeInfo.rd  = BITS(9, 7) + 8;
#ifdef KXEMU_ISA64
    this->gDecodeInfo.imm = BITS(12, 12) << 5 | BITS(6, 2);
#else
    this->gDecodeInfo.imm = BITS(6, 2);
#endif
}

void RVCore::decode_c_mvadd() {
    this->gDecodeInfo.rd  = BITS(11, 7);
    this->gDecodeInfo.rs2 = BITS( 6, 2);
}

void RVCore::decode_c_r() {
    this->gDecodeInfo.rd  = BITS(9, 7) + 8;
    this->gDecodeInfo.rs2 = BITS(4, 2) + 8;
}

#ifdef KXEMU_ISA64

void RVCore::decode_c_ldsp() {
    this->gDecodeInfo.rd  = BITS(11, 7);
    this->gDecodeInfo.imm = BITS(12, 12) << 5 | BITS(4, 2) << 6 | BITS(6, 5) << 3;
}

void RVCore::decode_c_sdsp() {
    this->gDecodeInfo.rs2 = BITS(6, 2);
    this->gDecodeInfo.imm = BITS(9, 7) << 6 | BITS(12, 10) << 3;
}

void RVCore::decode_c_ldsd() {
    this->gDecodeInfo.rd  = BITS(4, 2) + 8;
    this->gDecodeInfo.rs1 = BITS(9, 7) + 8;
    this->gDecodeInfo.rs2 = BITS(4, 2) + 8;
    this->gDecodeInfo.imm = BITS(6, 5) << 6 | BITS(12, 10) << 3;
}

#endif

void RVCore::decode_n() {}
