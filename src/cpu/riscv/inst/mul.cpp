#include "cpu/riscv/core.h"
#include "./local-decoder.h"
#include "cpu/word.h"
#include "isa/word.h"

#define RD  unsigned int rd  = BITS(11, 7);
#define RS1 unsigned int rs1 = BITS(19, 15);
#define RS2 unsigned int rs2 = BITS(24, 20);

using namespace kxemu::cpu;

void RVCore::do_mul() {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] * this->gpr[rs2]);
}

void RVCore::do_mulh() {
    RD; RS1; RS2;
    sdword_t res = (sdword_t)(sword_t)this->gpr[rs1] * (sword_t)this->gpr[rs2];
    this->set_gpr(rd, res >> WORD_BITS);
}

void RVCore::do_mulhsu() {
    RD; RS1; RS2;
    dword_t res = (sdword_t)this->gpr[rs1] * (dword_t)this->gpr[rs2];
    this->set_gpr(rd, res >> WORD_BITS);
}

void RVCore::do_mulhu() {
    RD; RS1; RS2;
    dword_t res = (dword_t)this->gpr[rs1] * (dword_t)this->gpr[rs2];
    this->set_gpr(rd, res >> WORD_BITS);
}

void RVCore::do_div() {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, -1);
    } else {
        this->set_gpr(rd, (sword_t)this->gpr[rs1] / (sword_t)this->gpr[rs2]);
    }
}

void RVCore::do_divu() {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, -1);
    } else {
        this->set_gpr(rd, this->gpr[rs1] / this->gpr[rs2]);
    }
}

void RVCore::do_rem() {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, rs1);
    } else {
        this->set_gpr(rd, (sword_t)this->gpr[rs1] % (sword_t)this->gpr[rs2]);
    }
}

void RVCore::do_remu() {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, rs2);
    } else {
        this->set_gpr(rd, this->gpr[rs1] % this->gpr[rs2]);
    }
}

#ifdef KXEMU_ISA64

void RVCore::do_mulw() {
    RD; RS1; RS2;
    int32_t res = (int32_t)this->gpr[rs1] * (int32_t)this->gpr[rs2];
    this->set_gpr(rd, res);
}

void RVCore::do_divw() {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, -1);
    } else {
        this->set_gpr(rd, (int32_t)this->gpr[rs1] / (int32_t)this->gpr[rs2]);
    }
}

void RVCore::do_divuw() {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, -1);
    } else {
        this->set_gpr(rd, (uint32_t)this->gpr[rs1] / (uint32_t)this->gpr[rs2]);
    }
}

void RVCore::do_remw() {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, rs1);
    } else {
        this->set_gpr(rd, (int32_t)this->gpr[rs1] % (int32_t)this->gpr[rs2]);
    }
}

void RVCore::do_remuw() {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, rs2);
    } else {
        this->set_gpr(rd, (uint32_t)this->gpr[rs1] % (uint32_t)this->gpr[rs2]);
    }
}

#endif
