#include "cpu/loongarch/core.h"
#include "./local-decoder.h"
#include "cpu/word.h"
#include "macro.h"

#include <cstdint>

using namespace kxemu::cpu;

#define RD unsigned int rd = BITS(4, 0)
#define RJ unsigned int rj = BITS(9, 5)
#define RK unsigned int rk = BITS(14, 10)

#define IMM_UI5  uint32_t imm = BITS(14, 10)
#define IMM_SI12 int32_t  imm = SEXT(BITS(21, 10), 12)
#define IMM_UI12 uint32_t imm = BITS(21, 10)
#define IMM_SI20 int32_t  imm = BITS(24, 5)
#define IMM_OFFS16 int32_t imm = SEXT(BITS(25, 10), 16) << 2
#define IMM_OFFS21 int32_t imm = SEXT(BITS(25, 10) | (BITS(9, 0) << 16), 26) << 2

void LACore::do_add_w() {
    RD; RJ; RK;
    uint32_t res = (uint32_t)gpr[rj] + (uint32_t)gpr[rk];
    this->set_gpr(rd, (sword_t)(int32_t)res);
}

void LACore::do_sub_w() {
    RD; RJ; RK;
    uint32_t res = (uint32_t)gpr[rj] - (uint32_t)gpr[rk];
    this->set_gpr(rd, (sword_t)(int32_t)res);
}

void LACore::do_slt() {
    RD; RJ; RK;
    this->set_gpr(rd, (sword_t)gpr[rj] < (sword_t)gpr[rk] ? 1 : 0);
}

void LACore::do_sltu() {
    RD; RJ; RK;
    this->set_gpr(rd, gpr[rj] < gpr[rk] ? 1 : 0);
}

void LACore::do_maskeqz() {
    RD; RJ; RK;
    this->set_gpr(rd, gpr[rk] == 0 ? 0 : rj);
}

void LACore::do_masknez() {
    RD; RJ; RK;
    this->set_gpr(rd, gpr[rk] != 0 ? 0 : rj);
}

void LACore::do_nor() {
    RD; RJ; RK;
    this->set_gpr(rd, ~(gpr[rj] | gpr[rk]));
}

void LACore::do_and() {
    RD; RJ; RK;
    this->set_gpr(rd, gpr[rj] & gpr[rk]);
}

void LACore::do_or() {
    RD; RJ; RK;
    this->set_gpr(rd, gpr[rj] | gpr[rk]);
}

void LACore::do_xor() {
    RD; RJ; RK;
    this->set_gpr(rd, gpr[rj] ^ gpr[rk]);
}

void LACore::do_orn() {
    RD; RJ; RK;
    this->set_gpr(rd, ~(gpr[rj] | ~gpr[rk]));
}

void LACore::do_andn() {
    RD; RJ; RK;
    this->set_gpr(rd, gpr[rj] & ~gpr[rk]);
}

void LACore::do_sll_w() {
    RD; RJ; RK;
    this->set_gpr(rd, (sword_t)(int32_t)(gpr[rj] << (gpr[rk] & 0x1f)));
}

void LACore::do_srl_w() {
    RD; RJ; RK;
    this->set_gpr(rd, (sword_t)(int32_t)((uint32_t)gpr[rj] >> (gpr[rk] & 0x1f)));
}

void LACore::do_sra_w() {
    RD; RJ; RK;
    this->set_gpr(rd, (sword_t)((int32_t)gpr[rj] >> (gpr[rk] & 0x1f)));
}

#ifdef KXEMU_ISA64

void LACore::do_add_d() {
    RD; RJ; RK;
    word_t res = gpr[rj] + gpr[rk];
    this->set_gpr(rd, res);
}

void LACore::do_sub_d() {
    RD; RJ; RK;
    word_t res = gpr[rj] - gpr[rk];
    this->set_gpr(rd, res);
}

#endif

void LACore::do_addi_w() {
    RD; RJ; IMM_SI12;
    int32_t res = (int32_t)gpr[rj] + imm;
    this->set_gpr(rd, (sword_t)res);
}

void LACore::do_slti() {
    RD; RJ; IMM_SI12;
    this->set_gpr(rd, (sword_t)gpr[rj] < (sword_t)imm ? 1 : 0);
}

void LACore::do_sltiu() {
    RD; RJ; IMM_SI12;
    this->set_gpr(rd, gpr[rj] < (word_t)(sword_t)imm ? 1 : 0);
}

void LACore::do_ori() {
    RD; RJ; IMM_UI12;
    this->set_gpr(rd, gpr[rj] | imm);
}

void LACore::do_xori() {
    RD; RJ; IMM_UI12;
    this->set_gpr(rd, gpr[rj] ^ imm);
}

void LACore::do_andi() {
    RD; RJ; IMM_UI12;
    this->set_gpr(rd, gpr[rj] & imm);
}

void LACore::do_slli_w() {
    RD; RJ; IMM_UI5;
    this->set_gpr(rd, (word_t)((int32_t)gpr[rj] << imm));
}

void LACore::do_srli_w() {
    RD; RJ; IMM_UI5;
    this->set_gpr(rd, (sword_t)(int32_t)((uint32_t)gpr[rj] >> imm));
}

void LACore::do_srai_w() {
    RD; RJ; IMM_UI5;
    this->set_gpr(rd, (sword_t)((int32_t)gpr[rj] >> imm));
}

void LACore::do_beq() {
    RD; RJ; IMM_OFFS16;
    if (unlikely(gpr[rd] == gpr[rj])) {
        this->npc = this->pc + imm;
    }
}

void LACore::do_bne() {
    RD; RJ; IMM_OFFS16;
    if (unlikely(gpr[rd] != gpr[rj])) {
        this->npc = this->pc + imm;
    }
}

void LACore::do_blt() {
    RD; RJ; IMM_OFFS16;
    if (unlikely((sword_t)gpr[rj] < (sword_t)gpr[rd])) {
        this->npc = this->pc + imm;
    }
}

void LACore::do_bltu() {
    RD; RJ; IMM_OFFS16;
    if (unlikely(gpr[rj] < gpr[rd])) {
        this->npc = this->pc + imm;
    }
}

void LACore::do_bge() {
    RD; RJ; IMM_OFFS16;
    if (unlikely((sword_t)gpr[rj] >= (sword_t)gpr[rd])) {
        this->npc = this->pc + imm;
    }
}

void LACore::do_bgeu() {
    RD; RJ; IMM_OFFS16;
    if (unlikely(gpr[rj] >= gpr[rd])) {
        this->npc = this->pc + imm;
    }
}

void LACore::do_b() {
    IMM_OFFS21;
    this->npc = this->pc + imm;
}

void LACore::do_bl() {
    IMM_OFFS21;
    gpr[1] = this->pc + 4;
    this->npc = this->pc + imm;
}

void LACore::do_jirl() {
    RD; RJ; IMM_OFFS16;
    this->set_gpr(rd, this->pc + 4);
    this->npc = gpr[rj] + imm;
}

void LACore::do_lu12i_w() {
    RD; IMM_SI20;
    this->set_gpr(rd, (sword_t)(imm << 12));
}

void LACore::do_pcaddu12i() {
    RD; IMM_SI20;
    this->set_gpr(rd, pc + (imm << 12));
}

void LACore::do_mul_w() {
    RD; RJ; RK;
    int64_t res = (int64_t)(int32_t)gpr[rj] * (int64_t)(int32_t)gpr[rk];
    this->set_gpr(rd, (sword_t)(res & 0xffffffff));
}

void LACore::do_mulh_w() {
    RD; RJ; RK;
    int64_t res = (int64_t)(int32_t)gpr[rj] * (int64_t)(int32_t)gpr[rk];
    this->set_gpr(rd, (sword_t)(res >> 32));
}

void LACore::do_mulh_wu() {
    RD; RJ; RK;
    int64_t res = (uint64_t)gpr[rj] * (uint64_t)gpr[rk];
    this->set_gpr(rd, (sword_t)(res >> 32));
}

void LACore::do_div_w() {
    RD; RJ; RK;
    if (unlikely(gpr[rk] == 0)) {
        this->set_gpr(rd, -1);
        return;
    }
    this->set_gpr(rd, (sword_t)((int32_t)gpr[rj] / (int32_t)gpr[rk]));
}

void LACore::do_div_wu() {
    RD; RJ; RK;
    if (unlikely(gpr[rk] == 0)) {
        this->set_gpr(rd, -1);
        return;
    }
    this->set_gpr(rd, (sword_t)(int32_t)(gpr[rj] / gpr[rk]));
}

void LACore::do_mod_w() {
    RD; RJ; RK;
    if (unlikely(gpr[rk] == 0)) {
        this->set_gpr(rd, gpr[rj]);
        return;
    }
    this->set_gpr(rd, (sword_t)((int32_t)gpr[rj] % (int32_t)gpr[rk]));
}

void LACore::do_mod_wu() {
    RD; RJ; RK;
    if (unlikely(gpr[rk] == 0)) {
        this->set_gpr(rd, gpr[rj]);
        return;
    }
    this->set_gpr(rd, (sword_t)(int32_t)(gpr[rj] % gpr[rk]));
}

void LACore::do_ld_b() {
    RD; RJ; IMM_SI12;
    bool success;
    word_t data = (int8_t)this->memory_load(gpr[rj] + imm, 1, success);
    if (success) {
        this->set_gpr(rd, data);
    }
}

void LACore::do_ld_h() {
    RD; RJ; IMM_SI12;
    bool success;
    sword_t data = (int16_t)this->memory_load(gpr[rj] + imm, 2, success);
    if (success) {
        this->set_gpr(rd, data);
    }
}

void LACore::do_ld_w() {
    RD; RJ; IMM_SI12;
    bool success;
    word_t data = (int32_t)this->memory_load(gpr[rj] + imm, 4, success);
    if (success) {
        this->set_gpr(rd, data);
    }
}

void LACore::do_ld_bu() {
    RD; RJ; IMM_SI12;
    bool success;
    word_t data = this->memory_load(gpr[rj] + imm, 1, success);
    if (success) {
        this->set_gpr(rd, data);
    }
}

void LACore::do_ld_hu() {
    RD; RJ; IMM_SI12;
    bool success;
    word_t data = this->memory_load(gpr[rj] + imm, 2, success);
    if (success) {
        this->set_gpr(rd, data);
    }
}

void LACore::do_st_b() {
    RD; RJ; IMM_SI12;
    this->memory_store(gpr[rj] + imm, gpr[rd], 1);
}

void LACore::do_st_h() {
    RD; RJ; IMM_SI12;
    this->memory_store(gpr[rj] + imm, gpr[rd], 2);
}

void LACore::do_st_w() {
    RD; RJ; IMM_SI12;
    this->memory_store(gpr[rj] + imm, gpr[rd], 4);
}

void LACore::do_break() {
    this->state = HALT;
    this->haltPC = this->pc;
    this->haltCode = gpr[4];
}
