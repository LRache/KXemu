#include "cpu/riscv/core.hpp"
#include "config/config.h"
#include "cpu/word.hpp"

#include <cstdint>

using namespace kxemu::cpu;

#define DECODE(name) void RVCore::decode_insttype_##name(DecodeInfo &decodeInfo)

#define BITS(hi, lo) ((word_t)((this->inst >> lo) & ((1 << (hi - lo + 1)) - 1))) // Extract bits from hi to lo
#define SEXT(bits, from) (sword_t)((int32_t)((int32_t)(bits) << (32 - (from))) >> (32 - (from))) // Signed extend

#ifdef CONFIG_DEBUG_DECODER
    #define set_field(field, v) decodeInfo.field = (v); decodeInfo.field##_set = true;
#else
    #define set_field(field, v) decodeInfo.field = (v);
#endif

#define  set_rd(v) set_field(rd,  ((v) | ((v) == 0) << 5))
#define set_frd(v) set_field(rd,  v)
#define set_rs1(v) set_field(rs1, v)
#define set_rs2(v) set_field(rs2, v)
#define set_rs3(v) set_field(rs3, v)
#define set_csr(v) set_field(csr, v)
#define set_imm(v) set_field(imm, v)
#define set_npc(v) set_field(npc, v)
#define set_flag(v) set_field(flag, v)

DECODE(r) {
    set_rd (BITS(11,  7));
    set_rs1(BITS(19, 15));
    set_rs2(BITS(24, 20));
}

DECODE(i) {
    set_rd (BITS(11,  7));
    set_rs1(BITS(19, 15));
    set_imm(SEXT(BITS(31, 20), 12));
}

DECODE(shifti) {
    set_rd (BITS(11,  7));
    set_rs1(BITS(19, 15));
#ifdef KXEMU_ISA64
    set_imm(BITS(25, 20));
#else
    set_imm(BITS(24, 20));
#endif
}

DECODE(shiftiw) {
    set_rd (BITS(11,  7));
    set_rs1(BITS(19, 15));
    set_imm(BITS(24, 20));
}

DECODE(s) {
    set_rs1(BITS(19, 15));
    set_rs2(BITS(24, 20));
    set_imm(SEXT((BITS(31, 25) << 5) | BITS(11, 7), 12));
}

DECODE(b) {
    set_rs1(BITS(19, 15));
    set_rs2(BITS(24, 20));
    set_npc(this->pc + SEXT((BITS(31, 31) << 12) | (BITS(30, 25) << 5) | (BITS(11, 8) << 1) | (BITS(7, 7) << 11), 13));
}

DECODE(j) {
    set_rd (BITS(11, 7));
    set_imm(SEXT((BITS(31, 31) << 20) | (BITS(30, 21) << 1) | (BITS(20, 20) << 11) | (BITS(19, 12) << 12), 21));
}

DECODE(auipc) {
    set_rd (BITS(11, 7));
    set_imm(this->pc + SEXT(BITS(31, 12) << 12, 32));
}

DECODE(lui) {
    set_rd (BITS(11, 7));
    set_imm(SEXT(BITS(31, 12) << 12, 32));
}

DECODE(i_f) {
    set_frd(BITS(11,  7));
    set_rs1(BITS(19, 15));
    set_imm(SEXT(BITS(31, 20), 12));
}

DECODE(r4_frm) {
    set_frd(BITS(11,  7));
    set_rs1(BITS(19, 15));
    set_rs2(BITS(24, 20));
    set_rs3(BITS(31, 27));
    set_flag(BITS(14, 12));
}

DECODE(r_frm) {
    set_frd(BITS(11,  7));
    set_rs1(BITS(19, 15));
    set_rs2(BITS(24, 20));
    set_flag(BITS(14, 12));
}

DECODE(r_f) {
    set_frd(BITS(11,  7));
    set_rs1(BITS(19, 15));
    set_rs2(BITS(24, 20));
}

DECODE(csrr) {
    set_rd (BITS(11,  7));
    set_rs1(BITS(19, 15));
    set_csr(BITS(31, 20));
}

DECODE(csri) {
    set_rd (BITS(11,  7));
    set_csr(BITS(31, 20));
    set_imm(BITS(19, 15));
}

DECODE(c_lwsp) {
    set_rd (BITS(11, 7));
    set_imm(BITS(12, 12) << 5 | BITS(3, 2) << 6 | BITS(6, 4) << 2)
}

DECODE(c_swsp) {
    set_rs2(BITS(6, 2));
    set_imm(BITS(8, 7) << 6 | BITS(12, 9) << 2);
}

DECODE(c_lwsw) {
    set_rd (BITS(4, 2) + 8);
    set_rs1(BITS(9, 7) + 8);
    set_rs2(BITS(4, 2) + 8);
    set_imm(BITS(5, 5) << 6 | BITS(12, 10) << 3 | BITS(6, 6) << 2);
}

DECODE(c_j) {
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
    set_npc(this->pc + imm);
}

DECODE(c_jalr) {
    set_rs1(BITS(11, 7));
    set_npc(this->pc + 2);
}

DECODE(c_b) {
    word_t imm = SEXT(
        BITS(12, 12) << 8 |
        BITS( 6,  5) << 6 |
        BITS( 2,  2) << 5 |
        BITS(11, 10) << 3 |
        BITS( 4,  3) << 1,
        9);
    
    set_rs1(BITS(9, 7) + 8);
    set_npc(this->pc + imm);
}

DECODE(c_li) {
    set_rd (BITS(11, 7));
    set_imm(SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6));
}

DECODE(c_lui) {
    set_rd (BITS(11, 7));
    set_imm(SEXT(BITS(12, 12) << 17 | BITS(6, 2) << 12, 18));
}

DECODE(c_addi) {
    set_rd (BITS(11, 7));
    set_imm(SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6));
}

DECODE(c_addi16sp) {
    word_t imm = SEXT(
        BITS(12, 12) << 9 |
        BITS( 4,  3) << 7 |
        BITS( 5,  5) << 6 |
        BITS( 2,  2) << 5 |
        BITS( 6,  6) << 4,
        10);
    
    set_imm(imm);
}

DECODE(c_addi4spn) {
    set_rd (BITS(4, 2) + 8);
    set_imm(BITS(10, 7) << 6 | BITS(12, 11) << 4 | BITS(5, 5) << 3 | BITS(6, 6) << 2);
}

DECODE(c_slli) {
    set_rd (BITS(11, 7));
    set_imm(BITS(12, 12) << 5 | BITS(6, 2));
}

DECODE(c_i) {
    set_rd (BITS(9, 7) + 8);
    set_imm(SEXT(BITS(12, 12) << 5 | BITS(6, 2), 6));
}

DECODE(c_shifti) {
    set_rd (BITS(9, 7) + 8);
#ifdef KXEMU_ISA64
    set_imm(BITS(12, 12) << 5 | BITS(6, 2));
#else
    set_imm(BITS(6, 2));
#endif
}

DECODE(c_mvadd) {
    set_rd (BITS(11, 7));
    set_rs2(BITS( 6, 2));
}

DECODE(c_r) {
    set_rd (BITS(9, 7) + 8);
    set_rs2(BITS(4, 2) + 8);
}

#ifdef KXEMU_ISA64

DECODE(c_ldsp) {
    set_rd (BITS(11, 7));
    set_imm(BITS(12, 12) << 5 | BITS(4, 2) << 6 | BITS(6, 5) << 3);
}

DECODE(c_sdsp) {
    set_rs2(BITS(6, 2));
    set_imm(BITS(9, 7) << 6 | BITS(12, 10) << 3);
}

DECODE(c_ldsd) {
    set_rd (BITS(4, 2) + 8);
    set_rs1(BITS(9, 7) + 8);
    set_rs2(BITS(4, 2) + 8);
    set_imm(BITS(6, 5) << 6 | BITS(12, 10) << 3);
}

#endif

DECODE(n) {}
