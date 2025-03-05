#include "cpu/riscv/core.h"
#include "./local-decoder.h"
#include "cpu/word.h"
#include "macro.h"
#include "word.h"
#include <cstdint>

using namespace kxemu::cpu;

void RVCore::do_mul(const DecodeInfo &decodeInfo) {
    DEST = SRC1 * SRC2;
}

void RVCore::do_mulh(const DecodeInfo &decodeInfo) {
    sdword_t res = (sdword_t)(sword_t)SRC1 * (sword_t)SRC2;
    DEST = res >> WORD_BITS;
}

void RVCore::do_mulhsu(const DecodeInfo &decodeInfo) {
    dword_t res = (sdword_t)SRC1 * (dword_t)SRC2;
    DEST = res >> WORD_BITS;
}

void RVCore::do_mulhu(const DecodeInfo &decodeInfo) {
    dword_t res = (dword_t)SRC1 * (dword_t)SRC2;
    DEST = res >> WORD_BITS;
}

void RVCore::do_div(const DecodeInfo &decodeInfo) {
    if (unlikely(SRC2 == 0)) {
        DEST = -1;
    } else {
        DEST = (sword_t)SRC1 / (sword_t)SRC2;
    }
}

void RVCore::do_divu(const DecodeInfo &decodeInfo) {
    if (unlikely(SRC2 == 0)) {
        DEST = -1;
    } else {
        DEST = SRC1 / SRC2;
    }
}

void RVCore::do_rem(const DecodeInfo &decodeInfo) {
    if (unlikely(SRC2 == 0)) {
        DEST = SRC1;
    } else {
        DEST = (sword_t)SRC1 % (sword_t)SRC2;
    }
}

void RVCore::do_remu(const DecodeInfo &decodeInfo) {
    if (unlikely(SRC2 == 0)) {
        DEST = SRC1;
    } else {
        DEST = SRC1 % SRC2;
    }
}

#ifdef KXEMU_ISA64

void RVCore::do_mulw(const DecodeInfo &decodeInfo) {
    DEST = (sword_t)((int32_t)SRC1 * (int32_t)SRC2);
}

void RVCore::do_divw(const DecodeInfo &decodeInfo) {
    if (unlikely(SRC2 == 0)) {
        DEST = -1;
    } else {
        DEST = (sword_t)((int32_t)SRC1 / (int32_t)SRC2);
    }
}

void RVCore::do_divuw(const DecodeInfo &decodeInfo) {
    if (unlikely(SRC2 == 0)) {
        DEST = -1;
    } else {
        DEST = (uint32_t)SRC1 / (uint32_t)SRC2;
    }
}

void RVCore::do_remw(const DecodeInfo &decodeInfo) {
    if (unlikely(SRC2 == 0)) {
        DEST = SRC1;
    } else {
        DEST = (sword_t)((int32_t)SRC1 % (int32_t)SRC2);
    }
}

void RVCore::do_remuw(const DecodeInfo &decodeInfo) {
    if (unlikely(SRC2 == 0)) {
        DEST = SRC1;
    } else {
        DEST = (sword_t)((int32_t)((uint32_t)SRC1 % (uint32_t)SRC2));
    }
}

#endif
