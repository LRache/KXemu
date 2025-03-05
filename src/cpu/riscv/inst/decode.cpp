#include "cpu/riscv/core.h"
#include "config/config.h"
#include "cpu/word.h"
#include "macro.h"
#include <cstdint>

using namespace kxemu::cpu;

#define BITS(hi, lo) ((word_t)((this->inst >> lo) & ((1 << (hi - lo + 1)) - 1))) // Extract bits from hi to lo
#define SEXT(bits, from) (sword_t)((int32_t)((bits) << (32 - (from))) >> (32 - (from))) // Signed extend

#ifdef CONFIG_DEBUG
    #define set_rd(v)  this->gDecodeInfo.rd  = unlikely((v) == 0) ? 32 : (v); this->gDecodeInfo.rd_set  = true;
    #define set_rs1(v) this->gDecodeInfo.rs1 = v; this->gDecodeInfo.rs1_set = true;
    #define set_rs2(v) this->gDecodeInfo.rs2 = v; this->gDecodeInfo.rs2_set = true;
    #define set_csr(v) this->gDecodeInfo.csr = v; this->gDecodeInfo.csr_set = true;
    #define set_imm(v) this->gDecodeInfo.imm = v; this->gDecodeInfo.imm_set = true;
#else
    #define set_rd(v)  this->gDecodeInfo.rd  = unlikely((v) == 0) ? 32 : (v);
    #define set_rs1(v) this->gDecodeInfo.rs1 = v;
    #define set_rs2(v) this->gDecodeInfo.rs2 = v;
    #define set_csr(v) this->gDecodeInfo.csr = v;
    #define set_imm(v) this->gDecodeInfo.imm = v;
#endif

void RVCore::decode_insttype_r() {
    // this->gDecodeInfo.rd  = BITS(11,  7);
    // this->gDecodeInfo.rs1 = BITS(19, 15);
    // this->gDecodeInfo.rs2 = BITS(24, 20);
    set_rd (BITS(11,  7));
    set_rs1(BITS(19, 15));
    set_rs2(BITS(24, 20));
}

void RVCore::decode_insttype_i() {
    // this->gDecodeInfo.rd  = BITS(11,  7);
    // this->gDecodeInfo.rs1 = BITS(19, 15);
    // this->gDecodeInfo.imm = SEXT(BITS(31, 20), 12);
    set_rd (BITS(11,  7));
    set_rs1(BITS(19, 15));
    set_imm(SEXT(BITS(31, 20), 12));
}

void RVCore::decode_insttype_shifti() {
//     this->gDecodeInfo.rd  = BITS(11,  7);
//     this->gDecodeInfo.rs1 = BITS(19, 15);
// #ifdef KXEMU_ISA64
//     this->gDecodeInfo.imm = BITS(25, 20);
// #else
//     this->gDecodeInfo.imm = BITS(24, 20);
// #endif
    set_rd (BITS(11,  7));
    set_rs1(BITS(19, 15));
#ifdef KXEMU_ISA64
    set_imm(BITS(25, 20));
#else
    set_imm(BITS(24, 20));
#endif
}

void RVCore::decode_insttype_s() {
    // this->gDecodeInfo.rs1 = BITS(19, 15);
    // this->gDecodeInfo.rs2 = BITS(24, 20);
    // this->gDecodeInfo.imm = SEXT((BITS(31, 25) << 5) | BITS(11, 7), 12);
    set_rs1(BITS(19, 15));
    set_rs2(BITS(24, 20));
    set_imm(SEXT((BITS(31, 25) << 5) | BITS(11, 7), 12));
}

void RVCore::decode_insttype_b() {
    // this->gDecodeInfo.rs1 = BITS(19, 15);
    // this->gDecodeInfo.rs2 = BITS(24, 20);
    // this->gDecodeInfo.imm = SEXT((BITS(31, 31) << 12) | (BITS(30, 25) << 5) | (BITS(11, 8) << 1) | (BITS(7, 7) << 11), 13);
    set_rs1(BITS(19, 15));
    set_rs2(BITS(24, 20));
    set_imm(SEXT((BITS(31, 31) << 12) | (BITS(30, 25) << 5) | (BITS(11, 8) << 1) | (BITS(7, 7) << 11), 13));
}

void RVCore::decode_insttype_j() {
    // this->gDecodeInfo.rd  = BITS(11,  7);
    // this->gDecodeInfo.imm = SEXT((BITS(31, 31) << 20) | (BITS(30, 21) << 1) | (BITS(20, 20) << 11) | (BITS(19, 12) << 12), 21);
    set_rd (BITS(11, 7));
    set_imm(SEXT((BITS(31, 31) << 20) | (BITS(30, 21) << 1) | (BITS(20, 20) << 11) | (BITS(19, 12) << 12), 21));
}

void RVCore::decode_insttype_u() {
    // this->gDecodeInfo.rd  = BITS(11, 7);
    // this->gDecodeInfo.imm = SEXT(BITS(31, 12) << 12, 32);
    set_rd (BITS(11, 7));
    set_imm(SEXT(BITS(31, 12) << 12, 32));
}

void RVCore::decode_insttype_csrr() {
    // this->gDecodeInfo.rd  = BITS(11,  7);
    // this->gDecodeInfo.rs1 = BITS(19, 15);
    // this->gDecodeInfo.csr = BITS(31, 20);
    set_rd (BITS(11,  7));
    set_rs1(BITS(19, 15));
    set_csr(BITS(31, 20));
}

void RVCore::decode_insttype_csri() {
    // this->gDecodeInfo.rd  = BITS(11,  7);
    // this->gDecodeInfo.csr = BITS(31, 20);
    // this->gDecodeInfo.imm = BITS(19, 15);
    set_rd (BITS(11,  7));
    set_csr(BITS(31, 20));
    set_imm(BITS(19, 15));
}

void RVCore::decode_insttype_c_lwsp() {
    // this->gDecodeInfo.rd  = BITS(11, 7);
    // this->gDecodeInfo.imm = BITS(12, 12) << 5 | BITS(3, 2) << 6 | BITS(6, 4) << 2;
    set_rd (BITS(11, 7));
    set_imm(BITS(12, 12) << 5 | BITS(3, 2) << 6 | BITS(6, 4) << 2)
}

void RVCore::decode_insttype_c_swsp() {
    // this->gDecodeInfo.rs1 = 2;
    // this->gDecodeInfo.rs2 = BITS(6, 2);
    // this->gDecodeInfo.imm = BITS(8, 7) << 6 | BITS(12, 9) << 2;
    set_rs2(BITS(6, 2));
    set_imm(BITS(8, 7) << 6 | BITS(12, 9) << 2);
}

void RVCore::decode_insttype_c_lwsw() {
    // this->gDecodeInfo.rd  = BITS(4, 2) + 8;
    // this->gDecodeInfo.rs1 = BITS(9, 7) + 8;
    // this->gDecodeInfo.rs2 = BITS(4, 2) + 8;
    // this->gDecodeInfo.imm = BITS(5, 5) << 6 | BITS(12, 10) << 3 | BITS(6, 6) << 2;
    set_rd (BITS(4, 2) + 8);
    set_rs1(BITS(9, 7) + 8);
    set_rs2(BITS(4, 2) + 8);
    set_imm(BITS(5, 5) << 6 | BITS(12, 10) << 3 | BITS(6, 6) << 2);
}

void RVCore::decode_insttype_c_j() {
    // this->gDecodeInfo.rs1 = BITS(11, 7);
    // this->gDecodeInfo.imm = SEXT(
    //     BITS(12, 12) << 11 |
    //     BITS( 8,  8) << 10 |
    //     BITS(10,  9) <<  8 |
    //     BITS( 7,  7) <<  6 |
    //     BITS( 6,  6) <<  7 |
    //     BITS( 2,  2) <<  5 |
    //     BITS(11, 11) <<  4 |
    //     BITS( 5,  3) <<  1,
    //     12);
    word_t imm = SEXT(
        BITS(12, 12) << 11 |
        BITS( 8,  8) << 10 |
        BITS(10,  9) <<  8 |
        BITS( 7,  7) <<  6 |
        BITS( 6,  6) <<  7 |
        BITS( 2,  2) <<  5 |
        BITS(11, 11) <<  4 |
        BITS( 5,  3) <<  1,
        12);
    
    set_rs1(BITS(11, 7));
    set_imm(imm);
}

void RVCore::decode_insttype_c_b() {
    // this->gDecodeInfo.rs1 = BITS(9, 7) + 8;
    // this->gDecodeInfo.imm = SEXT(
    //     BITS(12, 12) << 8 |
    //     BITS( 6,  5) << 6 |
    //     BITS( 2,  2) << 5 |
    //     BITS(11, 10) << 3 |
    //     BITS( 4,  3) << 1,
    //     9);

    word_t imm = SEXT(
        BITS(12, 12) << 8 |
        BITS( 6,  5) << 6 |
        BITS( 2,  2) << 5 |
        BITS(11, 10) << 3 |
        BITS( 4,  3) << 1,
        9);
    
    set_rs1(BITS(9, 7) + 8);
    set_imm(imm);
}

void RVCore::decode_insttype_c_li() {
    // this->gDecodeInfo.rd  = BITS(11, 7);
    // this->gDecodeInfo.imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);
    set_rd (BITS(11, 7));
    set_imm(SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6));
}

void RVCore::decode_insttype_c_lui() {
    // this->gDecodeInfo.rd  = BITS(11, 7);
    // this->gDecodeInfo.imm = SEXT(BITS(12, 12) << 17 | BITS(6, 2) << 12, 18);
    set_rd (BITS(11, 7));
    set_imm(SEXT(BITS(12, 12) << 17 | BITS(6, 2) << 12, 18));
}

void RVCore::decode_insttype_c_addi() {
    // this->gDecodeInfo.rd  = BITS(11, 7);
    // this->gDecodeInfo.imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);
    set_rd (BITS(11, 7));
    set_imm(SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6));
}

void RVCore::decode_insttype_c_addi16sp() {
    // this->gDecodeInfo.imm = SEXT(
    //     BITS(12, 12) << 9 |
    //     BITS( 4,  3) << 7 |
    //     BITS( 5,  5) << 6 |
    //     BITS( 2,  2) << 5 |
    //     BITS( 6,  6) << 4,
    //     10);
    word_t imm = SEXT(
        BITS(12, 12) << 9 |
        BITS( 4,  3) << 7 |
        BITS( 5,  5) << 6 |
        BITS( 2,  2) << 5 |
        BITS( 6,  6) << 4,
        10);
    
    set_imm(imm);
}

void RVCore::decode_insttype_c_addi4spn() {
    // this->gDecodeInfo.rd  = BITS(4, 2) + 8;
    // this->gDecodeInfo.imm = BITS(10, 7) << 6 | BITS(12, 11) << 4 | BITS(5, 5) << 3 | BITS(6, 6) << 2;

    set_rd (BITS(4, 2) + 8);
    set_imm(BITS(10, 7) << 6 | BITS(12, 11) << 4 | BITS(5, 5) << 3 | BITS(6, 6) << 2);
}

void RVCore::decode_insttype_c_slli() {
    // this->gDecodeInfo.rd  = BITS(11, 7);
    // this->gDecodeInfo.imm = BITS(12, 12) << 5 | BITS(6, 2);
    set_rd (BITS(11, 7));
    set_imm(BITS(12, 12) << 5 | BITS(6, 2));
}

void RVCore::decode_insttype_c_i() {
    // this->gDecodeInfo.rd  = BITS(9, 7) + 8;
    // this->gDecodeInfo.imm = SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6);
    set_rd (BITS(9, 7) + 8);
    set_imm(SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6));
}

void RVCore::decode_insttype_c_shifti() {
//     this->gDecodeInfo.rd  = BITS(9, 7) + 8;
// #ifdef KXEMU_ISA64
//     this->gDecodeInfo.imm = BITS(12, 12) << 5 | BITS(6, 2);
// #else
//     this->gDecodeInfo.imm = BITS(6, 2);
// #endif
    set_rd (BITS(9, 7) + 8);
#ifdef KXEMU_ISA64
    set_imm(BITS(12, 12) << 5 | BITS(6, 2));
#else
    set_imm(BITS(6, 2));
#endif
}

void RVCore::decode_insttype_c_mvadd() {
    // this->gDecodeInfo.rd  = BITS(11, 7);
    // this->gDecodeInfo.rs2 = BITS( 6, 2);
    set_rd (BITS(11, 7));
    set_rs2(BITS( 6, 2));
}

void RVCore::decode_insttype_c_r() {
    // this->gDecodeInfo.rd  = BITS(9, 7) + 8;
    // this->gDecodeInfo.rs2 = BITS(4, 2) + 8;
    set_rd (BITS(9, 7) + 8);
    set_rs2(BITS(4, 2) + 8);
}

#ifdef KXEMU_ISA64

void RVCore::decode_insttype_c_ldsp() {
    // this->gDecodeInfo.rd  = BITS(11, 7);
    // this->gDecodeInfo.imm = BITS(12, 12) << 5 | BITS(4, 2) << 6 | BITS(6, 5) << 3;
    set_rd (BITS(11, 7));
    set_imm(BITS(12, 12) << 5 | BITS(4, 2) << 6 | BITS(6, 5) << 3);
}

void RVCore::decode_insttype_c_sdsp() {
    // this->gDecodeInfo.rs2 = BITS(6, 2);
    // this->gDecodeInfo.imm = BITS(9, 7) << 6 | BITS(12, 10) << 3;
    set_rs2(BITS(6, 2));
    set_imm(BITS(9, 7) << 6 | BITS(12, 10) << 3);
}

void RVCore::decode_insttype_c_ldsd() {
    // this->gDecodeInfo.rd  = BITS(4, 2) + 8;
    // this->gDecodeInfo.rs1 = BITS(9, 7) + 8;
    // this->gDecodeInfo.rs2 = BITS(4, 2) + 8;
    // this->gDecodeInfo.imm = BITS(6, 5) << 6 | BITS(12, 10) << 3;
    set_rd (BITS(4, 2) + 8);
    set_rs1(BITS(9, 7) + 8);
    set_rs2(BITS(4, 2) + 8);
    set_imm(BITS(6, 5) << 6 | BITS(12, 10) << 3);
}

#endif

void RVCore::decode_insttype_n() {}
