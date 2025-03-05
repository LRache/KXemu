#include "cpu/riscv/core.h"
#include "./local-decoder.h"
#include "cpu/word.h"
#include "macro.h"
#include "word.h"
#include <cstdint>

using namespace kxemu::cpu;

void RVCore::do_mul(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // this->set_gpr(rd, this->gpr[rs1] * this->gpr[rs2]);
    DEST = SRC1 * SRC2;
}

void RVCore::do_mulh(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // sdword_t res = (sdword_t)(sword_t)this->gpr[rs1] * (sword_t)this->gpr[rs2];
    // this->set_gpr(rd, res >> WORD_BITS);
    sdword_t res = (sdword_t)(sword_t)SRC1 * (sword_t)SRC2;
    DEST = res >> WORD_BITS;
}

void RVCore::do_mulhsu(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // dword_t res = (sdword_t)this->gpr[rs1] * (dword_t)this->gpr[rs2];
    // this->set_gpr(rd, res >> WORD_BITS);
    dword_t res = (sdword_t)SRC1 * (dword_t)SRC2;
    DEST = res >> WORD_BITS;
}

void RVCore::do_mulhu(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // dword_t res = (dword_t)this->gpr[rs1] * (dword_t)this->gpr[rs2];
    // this->set_gpr(rd, res >> WORD_BITS);
    dword_t res = (dword_t)SRC1 * (dword_t)SRC2;
    DEST = res >> WORD_BITS;
}

void RVCore::do_div(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // if (this->gpr[rs2] == 0) {
    //     this->set_gpr(rd, -1);
    // } else {
    //     this->set_gpr(rd, (sword_t)this->gpr[rs1] / (sword_t)this->gpr[rs2]);
    // }
    if (unlikely(SRC2 == 0)) {
        DEST = -1;
    } else {
        DEST = (sword_t)SRC1 / (sword_t)SRC2;
    }
}

void RVCore::do_divu(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // if (this->gpr[rs2] == 0) {
    //     this->set_gpr(rd, -1);
    // } else {
    //     this->set_gpr(rd, this->gpr[rs1] / this->gpr[rs2]);
    // }
    if (unlikely(SRC2 == 0)) {
        DEST = -1;
    } else {
        DEST = SRC1 / SRC2;
    }
}

void RVCore::do_rem(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // if (this->gpr[rs2] == 0) {
    //     this->set_gpr(rd, rs1);
    // } else {
    //     this->set_gpr(rd, (sword_t)this->gpr[rs1] % (sword_t)this->gpr[rs2]);
    // }
    if (unlikely(SRC2 == 0)) {
        DEST = SRC1;
    } else {
        DEST = (sword_t)SRC1 % (sword_t)SRC2;
    }
}

void RVCore::do_remu(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // if (this->gpr[rs2] == 0) {
    //     this->set_gpr(rd, rs2);
    // } else {
    //     this->set_gpr(rd, this->gpr[rs1] % this->gpr[rs2]);
    // }
    if (unlikely(SRC2 == 0)) {
        DEST = SRC1;
    } else {
        DEST = SRC1 % SRC2;
    }
}

#ifdef KXEMU_ISA64

void RVCore::do_mulw(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // int32_t res = (int32_t)this->gpr[rs1] * (int32_t)this->gpr[rs2];
    // this->set_gpr(rd, res);
    DEST = (sword_t)((int32_t)SRC1 * (int32_t)SRC2);
}

void RVCore::do_divw(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // if (this->gpr[rs2] == 0) {
    //     this->set_gpr(rd, -1);
    // } else {
    //     this->set_gpr(rd, (int32_t)this->gpr[rs1] / (int32_t)this->gpr[rs2]);
    // }
    if (unlikely(SRC2 == 0)) {
        DEST = -1;
    } else {
        DEST = (sword_t)((int32_t)SRC1 / (int32_t)SRC2);
    }
}

void RVCore::do_divuw(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // if (this->gpr[rs2] == 0) {
    //     this->set_gpr(rd, -1);
    // } else {
    //     this->set_gpr(rd, (uint32_t)this->gpr[rs1] / (uint32_t)this->gpr[rs2]);
    // }
    if (unlikely(SRC2 == 0)) {
        DEST = -1;
    } else {
        DEST = (uint32_t)SRC1 / (uint32_t)SRC2;
    }
}

void RVCore::do_remw(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // if (this->gpr[rs2] == 0) {
    //     this->set_gpr(rd, rs1);
    // } else {
    //     this->set_gpr(rd, (int32_t)this->gpr[rs1] % (int32_t)this->gpr[rs2]);
    // }
    if (unlikely(SRC2 == 0)) {
        DEST = SRC1;
    } else {
        DEST = (sword_t)((int32_t)SRC1 % (int32_t)SRC2);
    }
}

void RVCore::do_remuw(const DecodeInfo &decodeInfo) {
    // RD; RS1; RS2;
    // if (this->gpr[rs2] == 0) {
    //     this->set_gpr(rd, rs2);
    // } else {
    //     this->set_gpr(rd, (uint32_t)this->gpr[rs1] % (uint32_t)this->gpr[rs2]);
    // }
    if (unlikely(SRC2 == 0)) {
        DEST = SRC1;
    } else {
        DEST = (sword_t)((int32_t)((uint32_t)SRC1 % (uint32_t)SRC2));
    }
}

#endif
