/*
 * Floating-point arithmetic on Berkeley SoftFloat (CONFIG_FPU_SOFT).
 *
 * The RISC-V specialization of SoftFloat maps rounding modes and
 * exception flags one-to-one onto the RISC-V encodings:
 *   frm   RNE/RTZ/RDN/RUP/RMM  =  softfloat_round_near_even/minMag/min/max/near_maxMag
 *   fflags NX/DZ/OF/UF/NX      =  softfloat_flag_invalid/infinite/overflow/underflow/inexact
 */
#include "cpu/riscv/core.hpp"
#include "cpu/riscv/csr-field.hpp"
#include "cpu/word.hpp"

#include <cstdint>
#include <cstring>

#ifdef CONFIG_FPU_SOFT

// SoftFloat is compiled as C and its headers have no C++ guards
extern "C" {
#include "softfloat.h"
#include "specialize.h"
#include "internals.h"
}

#include "./local-decoder.h"

using namespace kxemu::cpu;

// RISC-V requires tininess to be detected after rounding
namespace {
struct SoftFloatInit {
    SoftFloatInit() { softfloat_detectTininess = softfloat_tininess_afterRounding; }
};
SoftFloatInit softFloatInit;
} // namespace

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

static float32_t to_sf32(uint32_t u) {
    float32_t r;
    r.v = u;
    return r;
}

static float64_t to_sf64(uint64_t u) {
    float64_t r;
    r.v = u;
    return r;
}

#define SET_SOFT_RM \
    unsigned int rm = this->csr.fcsr.frm(); \
    if (rm == csr::FCSR::DYN) { \
        rm = decodeInfo.flag; \
        if (unlikely(rm == 5 || rm == 6)) { \
            this->do_invalid_inst(); \
            return; \
        } \
    } \
    softfloat_roundingMode = rm;

#define COLLECT_SOFT_FFLAGS \
    do { \
        if (unlikely(softfloat_exceptionFlags != 0)) { \
            this->raise_fflags(softfloat_exceptionFlags); \
            softfloat_exceptionFlags = 0; \
        } \
    } while (0)

// fmsub/fnmsub/fnmadd are fused multiply-adds with negated operands
#define SF32_NEG(u)   ((u) ^ 0x80000000)
#define SF64_NEG(u)   ((u) ^ 0x8000000000000000ull)

// RISC-V fmin/fmax: a NaN operand yields the other operand (canonical NaN
// if both are NaN); signaling NaNs raise the invalid flag; +0 > -0
static uint32_t fminmax_s_bits(uint32_t a, uint32_t b, bool isMax) {
    bool aNaN = isNaNF32UI(a), bNaN = isNaNF32UI(b);
    if (aNaN || bNaN) {
        if (softfloat_isSigNaNF32UI(a) || softfloat_isSigNaNF32UI(b)) {
            softfloat_raiseFlags(softfloat_flag_invalid);
        }
        if (aNaN && bNaN) return defaultNaNF32UI;
        return aNaN ? b : a;
    }
    if (((a | b) & 0x7fffffff) == 0) {
        return isMax ? ((a & 0x80000000) ? b : a) : ((a & 0x80000000) ? a : b);
    }
    bool aLTb = f32_lt(to_sf32(a), to_sf32(b));
    return isMax ? (aLTb ? b : a) : (aLTb ? a : b);
}

static uint64_t fminmax_d_bits(uint64_t a, uint64_t b, bool isMax) {
    bool aNaN = isNaNF64UI(a), bNaN = isNaNF64UI(b);
    if (aNaN || bNaN) {
        if (softfloat_isSigNaNF64UI(a) || softfloat_isSigNaNF64UI(b)) {
            softfloat_raiseFlags(softfloat_flag_invalid);
        }
        if (aNaN && bNaN) return defaultNaNF64UI;
        return aNaN ? b : a;
    }
    if (((a | b) & 0x7fffffffffffffff) == 0) {
        return isMax ? ((a & 0x8000000000000000ull) ? b : a) : ((a & 0x8000000000000000ull) ? a : b);
    }
    bool aLTb = f64_lt(to_sf64(a), to_sf64(b));
    return isMax ? (aLTb ? b : a) : (aLTb ? a : b);
}

void RVCore::do_fmadd_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint32_t r = f32_mulAdd(to_sf32(f32_bits(FSRC1S)), to_sf32(f32_bits(FSRC2S)), to_sf32(f32_bits(FSRC3S))).v;
    FDESTS = f32_from_bits(r);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fmsub_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint32_t r = f32_mulAdd(to_sf32(f32_bits(FSRC1S)), to_sf32(f32_bits(FSRC2S)),
                            to_sf32(SF32_NEG(f32_bits(FSRC3S)))).v;
    FDESTS = f32_from_bits(r);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fnmsub_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint32_t r = f32_mulAdd(to_sf32(SF32_NEG(f32_bits(FSRC1S))), to_sf32(f32_bits(FSRC2S)),
                            to_sf32(f32_bits(FSRC3S))).v;
    FDESTS = f32_from_bits(r);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fnmadd_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint32_t r = f32_mulAdd(to_sf32(SF32_NEG(f32_bits(FSRC1S))), to_sf32(f32_bits(FSRC2S)),
                            to_sf32(SF32_NEG(f32_bits(FSRC3S)))).v;
    FDESTS = f32_from_bits(r);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fadd_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint32_t r = f32_add(to_sf32(f32_bits(FSRC1S)), to_sf32(f32_bits(FSRC2S))).v;
    FDESTS = f32_from_bits(r);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fsub_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint32_t r = f32_sub(to_sf32(f32_bits(FSRC1S)), to_sf32(f32_bits(FSRC2S))).v;
    FDESTS = f32_from_bits(r);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fmul_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint32_t r = f32_mul(to_sf32(f32_bits(FSRC1S)), to_sf32(f32_bits(FSRC2S))).v;
    FDESTS = f32_from_bits(r);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fdiv_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint32_t r = f32_div(to_sf32(f32_bits(FSRC1S)), to_sf32(f32_bits(FSRC2S))).v;
    FDESTS = f32_from_bits(r);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fsqrt_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint32_t r = f32_sqrt(to_sf32(f32_bits(FSRC1S))).v;
    FDESTS = f32_from_bits(r);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fmin_s(const DecodeInfo &decodeInfo) {
    FDESTS = f32_from_bits(fminmax_s_bits(f32_bits(FSRC1S), f32_bits(FSRC2S), false));
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fmax_s(const DecodeInfo &decodeInfo) {
    FDESTS = f32_from_bits(fminmax_s_bits(f32_bits(FSRC1S), f32_bits(FSRC2S), true));
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

// f32_eq raises invalid on signaling NaN inputs, f32_lt/f32_le on any NaN
// inputs, matching the RISC-V feq/flt/fle semantics
void RVCore::do_feq_s(const DecodeInfo &decodeInfo) {
    DEST = f32_eq(to_sf32(f32_bits(FSRC1S)), to_sf32(f32_bits(FSRC2S))) ? 1 : 0;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_flt_s(const DecodeInfo &decodeInfo) {
    DEST = f32_lt(to_sf32(f32_bits(FSRC1S)), to_sf32(f32_bits(FSRC2S))) ? 1 : 0;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fle_s(const DecodeInfo &decodeInfo) {
    DEST = f32_le(to_sf32(f32_bits(FSRC1S)), to_sf32(f32_bits(FSRC2S))) ? 1 : 0;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_w_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    int32_t r = (int32_t)f32_to_i32(to_sf32(f32_bits(FSRC1S)), rm, true);
    DEST = (sword_t)r;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_wu_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint32_t r = (uint32_t)f32_to_ui32(to_sf32(f32_bits(FSRC1S)), rm, true);
    DEST = r;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_s_w(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    FDESTS = f32_from_bits(i32_to_f32((int32_t)SRC1).v);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_s_wu(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    FDESTS = f32_from_bits(ui32_to_f32((uint32_t)SRC1).v);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

#ifdef KXEMU_ISA64

void RVCore::do_fcvt_l_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    int64_t r = (int64_t)f32_to_i64(to_sf32(f32_bits(FSRC1S)), rm, true);
    DEST = (sword_t)r;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_lu_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint64_t r = (uint64_t)f32_to_ui64(to_sf32(f32_bits(FSRC1S)), rm, true);
    DEST = r;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_s_l(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    FDESTS = f32_from_bits(i64_to_f32((int64_t)SRC1).v);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_s_lu(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    FDESTS = f32_from_bits(ui64_to_f32(SRC1).v);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

#endif // KXEMU_ISA64

void RVCore::do_fmadd_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint64_t r = f64_mulAdd(to_sf64(f64_bits(FSRC1D)), to_sf64(f64_bits(FSRC2D)), to_sf64(f64_bits(FSRC3D))).v;
    FDESTD = f64_from_bits(r);

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fmsub_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint64_t r = f64_mulAdd(to_sf64(f64_bits(FSRC1D)), to_sf64(f64_bits(FSRC2D)),
                            to_sf64(SF64_NEG(f64_bits(FSRC3D)))).v;
    FDESTD = f64_from_bits(r);

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fnmsub_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint64_t r = f64_mulAdd(to_sf64(SF64_NEG(f64_bits(FSRC1D))), to_sf64(f64_bits(FSRC2D)),
                            to_sf64(f64_bits(FSRC3D))).v;
    FDESTD = f64_from_bits(r);

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fnmadd_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint64_t r = f64_mulAdd(to_sf64(SF64_NEG(f64_bits(FSRC1D))), to_sf64(f64_bits(FSRC2D)),
                            to_sf64(SF64_NEG(f64_bits(FSRC3D)))).v;
    FDESTD = f64_from_bits(r);

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fadd_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint64_t r = f64_add(to_sf64(f64_bits(FSRC1D)), to_sf64(f64_bits(FSRC2D))).v;
    FDESTD = f64_from_bits(r);

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fsub_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint64_t r = f64_sub(to_sf64(f64_bits(FSRC1D)), to_sf64(f64_bits(FSRC2D))).v;
    FDESTD = f64_from_bits(r);

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fmul_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint64_t r = f64_mul(to_sf64(f64_bits(FSRC1D)), to_sf64(f64_bits(FSRC2D))).v;
    FDESTD = f64_from_bits(r);

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fdiv_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint64_t r = f64_div(to_sf64(f64_bits(FSRC1D)), to_sf64(f64_bits(FSRC2D))).v;
    FDESTD = f64_from_bits(r);

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fsqrt_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint64_t r = f64_sqrt(to_sf64(f64_bits(FSRC1D))).v;
    FDESTD = f64_from_bits(r);

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fmin_d(const DecodeInfo &decodeInfo) {
    FDESTD = f64_from_bits(fminmax_d_bits(f64_bits(FSRC1D), f64_bits(FSRC2D), false));

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fmax_d(const DecodeInfo &decodeInfo) {
    FDESTD = f64_from_bits(fminmax_d_bits(f64_bits(FSRC1D), f64_bits(FSRC2D), true));

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_feq_d(const DecodeInfo &decodeInfo) {
    DEST = f64_eq(to_sf64(f64_bits(FSRC1D)), to_sf64(f64_bits(FSRC2D))) ? 1 : 0;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_flt_d(const DecodeInfo &decodeInfo) {
    DEST = f64_lt(to_sf64(f64_bits(FSRC1D)), to_sf64(f64_bits(FSRC2D))) ? 1 : 0;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fle_d(const DecodeInfo &decodeInfo) {
    DEST = f64_le(to_sf64(f64_bits(FSRC1D)), to_sf64(f64_bits(FSRC2D))) ? 1 : 0;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_s_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint32_t r = f64_to_f32(to_sf64(f64_bits(FSRC1D))).v;
    FDESTS = f32_from_bits(r);
    FPR_FILL_DEST_HIGH;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_d_s(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint64_t r = f32_to_f64(to_sf32(f32_bits(FSRC1S))).v;
    FDESTD = f64_from_bits(r);

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_w_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    int32_t r = (int32_t)f64_to_i32(to_sf64(f64_bits(FSRC1D)), rm, true);
    DEST = (sword_t)r;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_wu_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint32_t r = (uint32_t)f64_to_ui32(to_sf64(f64_bits(FSRC1D)), rm, true);
    DEST = r;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_d_w(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    FDESTD = f64_from_bits(i32_to_f64((int32_t)SRC1).v);

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_d_wu(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    FDESTD = f64_from_bits(ui32_to_f64((uint32_t)SRC1).v);

    COLLECT_SOFT_FFLAGS;
}

#ifdef KXEMU_ISA64

void RVCore::do_fcvt_l_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    int64_t r = (int64_t)f64_to_i64(to_sf64(f64_bits(FSRC1D)), rm, true);
    DEST = (sword_t)r;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_lu_d(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    uint64_t r = (uint64_t)f64_to_ui64(to_sf64(f64_bits(FSRC1D)), rm, true);
    DEST = r;

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_d_l(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    FDESTD = f64_from_bits(i64_to_f64((int64_t)SRC1).v);

    COLLECT_SOFT_FFLAGS;
}

void RVCore::do_fcvt_d_lu(const DecodeInfo &decodeInfo) {
    SET_SOFT_RM;

    FDESTD = f64_from_bits(ui64_to_f64(SRC1).v);

    COLLECT_SOFT_FFLAGS;
}

#endif // KXEMU_ISA64

#endif // CONFIG_FPU_SOFT
