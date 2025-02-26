#include "cpu/riscv/core.h"
#include "cpu/word.h"
#include <cstdint>

using namespace kxemu::cpu;

#define BITS(hi, lo) ((word_t)((this->inst >> lo) & ((1 << (hi - lo + 1)) - 1))) // Extract bits from hi to lo
#define SEXT(bits, from) (sword_t)((int32_t)((bits) << (32 - (from))) >> (32 - (from))) // Signed extend

void RVCore::decode_r() {
    this->decodeInfo.rd  = BITS(11,  7);
    this->decodeInfo.rs1 = BITS(19, 15);
    this->decodeInfo.rs2 = BITS(24, 20);
}

void RVCore::decode_i() {
    this->decodeInfo.rd  = BITS(11,  7);
    this->decodeInfo.rs1 = BITS(19, 15);
    this->decodeInfo.imm = SEXT(BITS(31, 20), 12);
}

void RVCore::decode_shifti() {
    this->decodeInfo.rd  = BITS(11,  7);
    this->decodeInfo.rs1 = BITS(19, 15);
#ifdef KXEMU_ISA64
    this->decodeInfo.imm = BITS(25, 20);
#else
    this->decodeInfo.imm = BITS(24, 20);
#endif
}

void RVCore::decode_s() {
    this->decodeInfo.rs1 = BITS(19, 15);
    this->decodeInfo.rs2 = BITS(24, 20);
    this->decodeInfo.imm = SEXT((BITS(31, 25) << 5) | BITS(11, 7), 12);
}

void RVCore::decode_b() {
    this->decodeInfo.rs1 = BITS(19, 15);
    this->decodeInfo.rs2 = BITS(24, 20);
    this->decodeInfo.imm = SEXT((BITS(31, 31) << 12) | (BITS(30, 25) << 5) | (BITS(11, 8) << 1) | (BITS(7, 7) << 11), 13);
}

void RVCore::decode_j() {
    this->decodeInfo.rd  = BITS(11,  7);
    this->decodeInfo.imm = SEXT((BITS(31, 31) << 20) | (BITS(30, 21) << 1) | (BITS(20, 20) << 11) | (BITS(19, 12) << 12), 21);
}

void RVCore::decode_u() {
    this->decodeInfo.rd  = BITS(11, 7);
    this->decodeInfo.imm = SEXT(BITS(31, 12) << 12, 32);
}

void RVCore::decode_csrr() {
    this->decodeInfo.rd  = BITS(11,  7);
    this->decodeInfo.rs1 = BITS(19, 15);
    this->decodeInfo.csr = BITS(31, 20);
}

void RVCore::decode_csri() {
    this->decodeInfo.rd  = BITS(11,  7);
    this->decodeInfo.csr = BITS(31, 20);
    this->decodeInfo.imm = BITS(19, 15);
}

void RVCore::decode_c_lwsp() {
    this->decodeInfo.rd  = BITS(11, 7);
    this->decodeInfo.imm = BITS(12, 12) << 5 | BITS(3, 2) << 6 | BITS(6, 4) << 2;
}

void RVCore::decode_c_swsp() {
    this->decodeInfo.rs1 = 2;
    this->decodeInfo.rs2 = BITS(6, 2);
    this->decodeInfo.imm = BITS(8, 7) << 6 | BITS(12, 9) << 2;
}

void RVCore::decode_c_lwsw() {
    this->decodeInfo.rd  = BITS(4, 2) + 8;
    this->decodeInfo.rs1 = BITS(9, 7) + 8;
    this->decodeInfo.rs2 = BITS(4, 2) + 8;
    this->decodeInfo.imm = BITS(5, 5) << 6 | BITS(12, 10) << 3 | BITS(6, 6) << 2;
}

void RVCore::decode_c_j() {
    this->decodeInfo.rs1 = BITS(11, 7);
    this->decodeInfo.imm = SEXT(
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
    this->decodeInfo.rs1 = BITS(9, 7) + 8;
    this->decodeInfo.imm = SEXT(
        BITS(12, 12) << 8 |
        BITS( 6,  5) << 6 |
        BITS( 2,  2) << 5 |
        BITS(11, 10) << 3 |
        BITS( 4,  3) << 1,
        9);
}

void RVCore::decode_c_li() {
    this->decodeInfo.rd  = BITS(11, 7);
    this->decodeInfo.imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);
}

void RVCore::decode_c_lui() {
    this->decodeInfo.rd  = BITS(11, 7);
    this->decodeInfo.imm = SEXT(BITS(12, 12) << 17 | BITS(6, 2) << 12, 18);
}

void RVCore::decode_c_addi() {
    this->decodeInfo.rd  = BITS(11, 7);
    this->decodeInfo.imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);
}

void RVCore::decode_c_addi16sp() {
    this->decodeInfo.imm = SEXT(
        BITS(12, 12) << 9 |
        BITS( 4,  3) << 7 |
        BITS( 5,  5) << 6 |
        BITS( 2,  2) << 5 |
        BITS( 6,  6) << 4,
        10);
}

void RVCore::decode_c_addi4spn() {
    this->decodeInfo.rd  = BITS(4, 2) + 8;
    this->decodeInfo.imm = BITS(10, 7) << 6 | BITS(12, 11) << 4 | BITS(5, 5) << 3 | BITS(6, 6) << 2;
}

void RVCore::decode_c_slli() {
    this->decodeInfo.rd  = BITS(11, 7);
    this->decodeInfo.imm = BITS(12, 12) << 5 | BITS(6, 2);
}

void RVCore::decode_c_i() {
    this->decodeInfo.rd  = BITS(9, 7) + 8;
    this->decodeInfo.imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);
}

void RVCore::decode_c_shifti() {
    this->decodeInfo.rd  = BITS(9, 7) + 8;
#ifdef KXEMU_ISA64
    this->decodeInfo.imm = BITS(12, 12) << 5 | BITS(6, 2);
#else
    this->decodeInfo.imm = BITS(6, 2);
#endif
}

void RVCore::decode_c_mvadd() {
    this->decodeInfo.rd  = BITS(11, 7);
    this->decodeInfo.rs2 = BITS( 6, 2);
}

void RVCore::decode_c_r() {
    this->decodeInfo.rd  = BITS(9, 7) + 8;
    this->decodeInfo.rs2 = BITS(4, 2) + 8;
}

#ifdef KXEMU_ISA64

void RVCore::decode_c_ldsp() {
    this->decodeInfo.rd  = BITS(11, 7);
    this->decodeInfo.imm = BITS(12, 12) << 5 | BITS(4, 2) << 6 | BITS(6, 5) << 3;
}

void RVCore::decode_c_sdsp() {
    this->decodeInfo.rs2 = BITS(6, 2);
    this->decodeInfo.imm = BITS(9, 7) << 6 | BITS(12, 10) << 3;
}

void RVCore::decode_c_ldsd() {
    this->decodeInfo.rd  = BITS(4, 2) + 8;
    this->decodeInfo.rs1 = BITS(9, 7) + 8;
    this->decodeInfo.rs2 = BITS(4, 2) + 8;
    this->decodeInfo.imm = BITS(6, 5) << 6 | BITS(12, 10) << 3;
}

#endif

void RVCore::decode_n() {}
