/*
 * Shared floating-point implementation.
 *
 * Arithmetic instructions are provided by float-host.cpp (CONFIG_FPU_HOST)
 * or float-soft.cpp (CONFIG_FPU_SOFT). When CONFIG_FPU_NONE is selected all
 * floating-point instructions trap as illegal instructions (float-none.cpp).
 */
#include "cpu/riscv/core.hpp"
#include "cpu/riscv/csr-field.hpp"
#include "cpu/word.hpp"

#include <cstdint>
#include <cstring>

#include "./local-decoder.h"

using namespace kxemu::cpu;

void RVCore::raise_fflags(word_t flags) {
    this->csr.fcsr.set_fflags(this->csr.fcsr.fflags() | flags);
}

#ifndef CONFIG_FPU_NONE

static uint32_t f32_bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, sizeof(u));
    return u;
}

static float f32_from_bits(uint32_t u) {
    float f;
    std::memcpy(&f, &u, sizeof(f));
    return f;
}

static uint64_t f64_bits(double d) {
    uint64_t u;
    std::memcpy(&u, &d, sizeof(u));
    return u;
}

static double f64_from_bits(uint64_t u) {
    double d;
    std::memcpy(&d, &u, sizeof(d));
    return d;
}

void RVCore::do_flw(const DecodeInfo &decodeInfo) {
    uint32_t data = this->memory_load(SRC1 + IMM, 4);
    FDESTS = f32_from_bits(data);
    FPR_FILL_DEST_HIGH;
}

void RVCore::do_fsw(const DecodeInfo &decodeInfo) {
    this->memory_store(SRC1 + IMM, f32_bits(FSRC2S), 4);
}

void RVCore::do_fld(const DecodeInfo &decodeInfo) {
    uint64_t data = this->memory_load(SRC1 + IMM, 8);
    FDESTD = f64_from_bits(data);
}

void RVCore::do_fsd(const DecodeInfo &decodeInfo) {
    this->memory_store(SRC1 + IMM, f64_bits(FSRC2D), 8);
}

void RVCore::do_fmv_x_w(const DecodeInfo &decodeInfo) {
    DEST = f32_bits(FSRC1S);

    #ifdef KXEMU_ISA64
    if (DEST & 0x80000000) {
        DEST |= 0xffffffff00000000;
    }
    #endif
}

void RVCore::do_fmv_w_x(const DecodeInfo &decodeInfo) {
    FDESTS = f32_from_bits(SRC1);
    FPR_FILL_DEST_HIGH;
}

void RVCore::do_fsgnj_s(const DecodeInfo &decodeInfo) {
    FDESTS = f32_from_bits((f32_bits(FSRC1S) & 0x7fffffff) | (f32_bits(FSRC2S) & 0x80000000));
    FPR_FILL_DEST_HIGH;
}

void RVCore::do_fsgnjn_s(const DecodeInfo &decodeInfo) {
    FDESTS = f32_from_bits((f32_bits(FSRC1S) & 0x7fffffff) | (~f32_bits(FSRC2S) & 0x80000000));
    FPR_FILL_DEST_HIGH;
}

void RVCore::do_fsgnjx_s(const DecodeInfo &decodeInfo) {
    FDESTS = f32_from_bits(f32_bits(FSRC1S) ^ (f32_bits(FSRC2S) & 0x80000000));
    FPR_FILL_DEST_HIGH;
}

void RVCore::do_fclass_s(const DecodeInfo &decodeInfo) {
    uint32_t bits = f32_bits(FSRC1S);
    uint32_t exp  = (bits >> 23) & 0xff;
    uint32_t frac = bits & 0x007fffff;
    if (exp == 0xff) {
        if (frac == 0) {
            DEST = (bits >> 31) ? 0 : 7; // -inf / +inf
        } else {
            DEST = (frac & 0x00400000) ? 8 : 9; // quiet NaN / signaling NaN
        }
    } else if (exp == 0) {
        if (frac == 0) {
            DEST = (bits >> 31) ? 3 : 4; // -zero / +zero
        } else {
            DEST = (bits >> 31) ? 2 : 5; // -subnormal / +subnormal
        }
    } else {
        DEST = (bits >> 31) ? 1 : 6; // -normal / +normal
    }
}

#ifdef KXEMU_ISA64

void RVCore::do_fmv_x_d(const DecodeInfo &decodeInfo) {
    DEST = f64_bits(FSRC1D);
}

void RVCore::do_fmv_d_x(const DecodeInfo &decodeInfo) {
    FDESTD = f64_from_bits(SRC1);
}

#endif // KXEMU_ISA64

void RVCore::do_fsgnj_d(const DecodeInfo &decodeInfo) {
    FDESTD = f64_from_bits((f64_bits(FSRC1D) & 0x7fffffffffffffff) | (f64_bits(FSRC2D) & 0x8000000000000000));
}

void RVCore::do_fsgnjn_d(const DecodeInfo &decodeInfo) {
    FDESTD = f64_from_bits((f64_bits(FSRC1D) & 0x7fffffffffffffff) | (~f64_bits(FSRC2D) & 0x8000000000000000));
}

void RVCore::do_fsgnjx_d(const DecodeInfo &decodeInfo) {
    FDESTD = f64_from_bits(f64_bits(FSRC1D) ^ (f64_bits(FSRC2D) & 0x8000000000000000));
}

void RVCore::do_fclass_d(const DecodeInfo &decodeInfo) {
    uint64_t bits = f64_bits(FSRC1D);
    uint32_t exp  = (bits >> 52) & 0x7ff;
    uint32_t frac = (bits & 0x000fffffffffffff) != 0;
    if (exp == 0x7ff) {
        if (frac == 0) {
            DEST = (bits >> 63) ? 0 : 7; // -inf / +inf
        } else {
            DEST = (bits & 0x0008000000000000) ? 8 : 9; // quiet NaN / signaling NaN
        }
    } else if (exp == 0) {
        if (frac == 0) {
            DEST = (bits >> 63) ? 3 : 4; // -zero / +zero
        } else {
            DEST = (bits >> 63) ? 2 : 5; // -subnormal / +subnormal
        }
    } else {
        DEST = (bits >> 63) ? 1 : 6; // -normal / +normal
    }
}

#endif // CONFIG_FPU_NONE
