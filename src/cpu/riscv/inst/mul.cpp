#include "cpu/riscv/core.h"
#include "./local-decoder.h"
#include "cpu/word.h"
#include "word.h"

#define RD  unsigned int rd  = decodeInfo.rd
#define RS1 unsigned int rs1 = decodeInfo.rs1
#define RS2 unsigned int rs2 = decodeInfo.rs2

using namespace kxemu::cpu;

void RVCore::do_mul(const DecodeInfo &decodeInfo) {
    RD; RS1; RS2;
    this->set_gpr(rd, this->gpr[rs1] * this->gpr[rs2]);
}

void RVCore::do_mulh(const DecodeInfo &decodeInfo) {
    RD; RS1; RS2;
    sdword_t res = (sdword_t)(sword_t)this->gpr[rs1] * (sword_t)this->gpr[rs2];
    this->set_gpr(rd, res >> WORD_BITS);
}

void RVCore::do_mulhsu(const DecodeInfo &decodeInfo) {
    RD; RS1; RS2;
    dword_t res = (sdword_t)this->gpr[rs1] * (dword_t)this->gpr[rs2];
    this->set_gpr(rd, res >> WORD_BITS);
}

void RVCore::do_mulhu(const DecodeInfo &decodeInfo) {
    RD; RS1; RS2;
    dword_t res = (dword_t)this->gpr[rs1] * (dword_t)this->gpr[rs2];
    this->set_gpr(rd, res >> WORD_BITS);
}

void RVCore::do_div(const DecodeInfo &decodeInfo) {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, -1);
    } else {
        this->set_gpr(rd, (sword_t)this->gpr[rs1] / (sword_t)this->gpr[rs2]);
    }
}

void RVCore::do_divu(const DecodeInfo &decodeInfo) {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, -1);
    } else {
        this->set_gpr(rd, this->gpr[rs1] / this->gpr[rs2]);
    }
}

void RVCore::do_rem(const DecodeInfo &decodeInfo) {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, rs1);
    } else {
        this->set_gpr(rd, (sword_t)this->gpr[rs1] % (sword_t)this->gpr[rs2]);
    }
}

void RVCore::do_remu(const DecodeInfo &decodeInfo) {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, rs2);
    } else {
        this->set_gpr(rd, this->gpr[rs1] % this->gpr[rs2]);
    }
}

#ifdef KXEMU_ISA64

void RVCore::do_mulw(const DecodeInfo &decodeInfo) {
    RD; RS1; RS2;
    int32_t res = (int32_t)this->gpr[rs1] * (int32_t)this->gpr[rs2];
    this->set_gpr(rd, res);
}

void RVCore::do_divw(const DecodeInfo &decodeInfo) {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, -1);
    } else {
        this->set_gpr(rd, (int32_t)this->gpr[rs1] / (int32_t)this->gpr[rs2]);
    }
}

void RVCore::do_divuw(const DecodeInfo &decodeInfo) {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, -1);
    } else {
        this->set_gpr(rd, (uint32_t)this->gpr[rs1] / (uint32_t)this->gpr[rs2]);
    }
}

void RVCore::do_remw(const DecodeInfo &decodeInfo) {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, rs1);
    } else {
        this->set_gpr(rd, (int32_t)this->gpr[rs1] % (int32_t)this->gpr[rs2]);
    }
}

void RVCore::do_remuw(const DecodeInfo &decodeInfo) {
    RD; RS1; RS2;
    if (this->gpr[rs2] == 0) {
        this->set_gpr(rd, rs2);
    } else {
        this->set_gpr(rd, (uint32_t)this->gpr[rs1] % (uint32_t)this->gpr[rs2]);
    }
}

#endif
