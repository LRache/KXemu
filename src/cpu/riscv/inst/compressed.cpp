#include "cpu/riscv/core.h"
#include "./local-decoder.h"
#include "cpu/word.h"
#include "macro.h"


#define REQUIRE(cond) do {if (unlikely(!(cond))) { this->do_invalid_inst(); return; }} while(0);
#define REQUIRE_NOT_x0(reg)  REQUIRE(!(reg##_is_x0))
#define REQUIRE_NOT_ZERO(v)  REQUIRE((v) != 0)

#define SP (this->gpr[2])

using namespace kxemu::cpu;

void RVCore::do_c_lwsp(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    REQUIRE_NOT_x0(rd);

    DEST = SEXT64(this->memory_load(SP + IMM, 4));
}

void RVCore::do_c_swsp(const DecodeInfo &decodeInfo) {
    TAG_RS2; TAG_IMM;
    
    this->memory_store(SP + IMM, SRC2, 4);
}

void RVCore::do_c_lw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS1; TAG_IMM;
    
    DEST = SEXT64(this->memory_load(SRC1 + IMM, 4));
}

void RVCore::do_c_sw(const DecodeInfo &decodeInfo) {
    TAG_RS1; TAG_RS2; TAG_IMM;
    
    this->memory_store(SRC1 + IMM, SRC2, 4);
}

void RVCore::do_c_j(const DecodeInfo &decodeInfo) {
    TAG_IMM;
    
    this->npc = this->pc + IMM;
}

void RVCore::do_c_jal(const DecodeInfo &decodeInfo) {
    TAG_IMM;
    
    this->gpr[1] = this->pc + 2;
    this->npc = this->pc + IMM;
}

void RVCore::do_c_jr(const DecodeInfo &decodeInfo) {
    TAG_RS1;
    
    REQUIRE_NOT_x0(rs1);

    this->npc = SRC1;
}

void RVCore::do_c_jalr(const DecodeInfo &decodeInfo) {
    TAG_RS1;
    
    this->gpr[1] = this->pc + 2;
    this->npc = SRC1;
}

void RVCore::do_c_beqz(const DecodeInfo &decodeInfo) {
    TAG_RS1; TAG_IMM;
    
    if (unlikely(SRC1 == 0)) {
        this->npc = this->pc + IMM;
    }
}

void RVCore::do_c_bnez(const DecodeInfo &decodeInfo) {
    TAG_RS1; TAG_IMM;
    
    if (unlikely(SRC1 != 0)) {
        this->npc = this->pc + IMM;
    }
}

void RVCore::do_c_li(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    REQUIRE_NOT_x0(rd);
    
    DEST = IMM;
}

void RVCore::do_c_lui(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    REQUIRE_NOT_x0(rd);
    REQUIRE_NOT_ZERO(IMM);

    DEST = IMM;
}

void RVCore::do_c_addi(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    DEST = DEST + IMM;
}

void RVCore::do_c_addi16sp(const DecodeInfo &decodeInfo) {;
    TAG_IMM;
    
    REQUIRE_NOT_ZERO(IMM);

    SP = SP + IMM;
}

void RVCore::do_c_addi4spn(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    REQUIRE_NOT_ZERO(IMM);

    DEST = SP + IMM;
}

void RVCore::do_c_slli(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    REQUIRE_NOT_x0(rd);

    DEST = DEST << IMM;
}

void RVCore::do_c_srli(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    DEST = (word_t)DEST >> IMM;
}

void RVCore::do_c_srai(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    DEST = (sword_t)DEST >> IMM;
}

void RVCore::do_c_andi(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    DEST = DEST & IMM;
}

void RVCore::do_c_mv(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS2;
    
    REQUIRE_NOT_x0(rd);

    DEST = SRC2;
}

void RVCore::do_c_add(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS2;
    
    REQUIRE_NOT_x0(rd);

    DEST = DEST + SRC2;
}

void RVCore::do_c_sub(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS2;
    
    DEST = DEST - SRC2;
}

void RVCore::do_c_xor(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS2;
    
    DEST = DEST ^ SRC2;
}

void RVCore::do_c_or(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS2;
    
    DEST = DEST | SRC2;
}

void RVCore::do_c_and(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS2;
    
    DEST = DEST & SRC2;
}

void RVCore::do_c_nop(const DecodeInfo &) {
    // Do nothing
}

#ifdef KXEMU_ISA64

void RVCore::do_c_ldsp(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    REQUIRE_NOT_x0(rd);

    DEST = this->memory_load(this->gpr[2] + IMM, 8);
}

void RVCore::do_c_sdsp(const DecodeInfo &decodeInfo) {
    TAG_RS2; TAG_IMM;
    
    this->memory_store(this->gpr[2] + IMM, SRC2, 8);
}

void RVCore::do_c_ld(const DecodeInfo &decodeInfo) {
    TAG_RS1; TAG_IMM;
    
    DEST = this->memory_load(SRC1 + IMM, 8);
}

void RVCore::do_c_sd(const DecodeInfo &decodeInfo) {
    TAG_RS1; TAG_RS2; TAG_IMM;
    
    this->memory_store(SRC1 + IMM, SRC2, 8);
}

void RVCore::do_c_addiw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_IMM;
    
    REQUIRE_NOT_x0(rd);

    DEST = SEXT64((int32_t)DEST + IMM);
}

void RVCore::do_c_addw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS2;
    
    DEST = SEXT64((int32_t)DEST + (int32_t)SRC2);
}

void RVCore::do_c_subw(const DecodeInfo &decodeInfo) {
    TAG_RD; TAG_RS2;
    
    DEST = SEXT64((int32_t)DEST - (int32_t)SRC2);
}

#endif
