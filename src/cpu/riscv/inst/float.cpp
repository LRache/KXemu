#include "cpu/riscv/core.h"
#include "cpu/riscv/def.h"

#include <cfenv>
#include <cstdint>
#include <cstring>
#include <cmath>

#include "./local-decoder.h"

using namespace kxemu::cpu;

#define FILL_DEST_HIGH this->fpr[decodeInfo.rd].high = -1;

static void set_fpu_rounding_mode(unsigned int rm) {
    switch (rm) {
        case FRM_RNE:
            fesetround(FE_TONEAREST);
            break;
        case FRM_RTZ:
            fesetround(FE_TOWARDZERO);
            break;
        case FRM_RDN:
            fesetround(FE_DOWNWARD);
            break;
        case FRM_RUP:
            fesetround(FE_UPWARD);
            break;
        case FRM_RMM:
            fesetround(FE_TONEAREST);
            break;
        case FRM_DYN:
            break;
    }
}

#define SET_FPU \
    unsigned int rm = this->frm; \
    if (rm == FRM_DYN) { \
        rm = decodeInfo.flag; \
        if (unlikely(rm == 5 || rm == 6)) { \
            this->do_invalid_inst(); \
            return; \
        } \
    } \
    int old = fegetround(); \
    set_fpu_rounding_mode(rm);

#define RESTORE_FPU fesetround(old);

void RVCore::update_fcsr() {
    this->frm = FCSR_RM(this->csr.read_csr(CSR_FCSR));
}

void RVCore::do_flw(const DecodeInfo &decodeInfo) {
    uint32_t i = this->memory_load(SRC1 + IMM, 4);
    float f;
    std::memcpy(&f, &i, 4);
    FDESTS = f;
    FILL_DEST_HIGH;
}

void RVCore::do_fsw(const DecodeInfo &decodeInfo) {
    uint32_t i;
    std::memcpy(&i, &FSRC2S, 4);
    this->memory_store(SRC1 + IMM, i, 4);
}

void RVCore::do_fmadd_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = FSRC1S * FSRC2S + FSRC3S;
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

void RVCore::do_fmsub_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = FSRC1S * FSRC2S - FSRC3S;
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

void RVCore::do_fnmsub_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = -FSRC1S * FSRC2S + FSRC3S;
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

void RVCore::do_fnmadd_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = -FSRC1S * FSRC2S - FSRC3S;
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

void RVCore::do_fadd_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = FSRC1S + FSRC2S;
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

void RVCore::do_fsub_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = FSRC1S - FSRC2S;
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

void RVCore::do_fmul_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = FSRC1S * FSRC2S;
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

void RVCore::do_fdiv_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = FSRC1S / FSRC2S;
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

void RVCore::do_fsqrt_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = std::sqrt(FSRC1S);
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

void RVCore::do_fsgnj_s(const DecodeInfo &decodeInfo) {
    int sign = std::signbit(FSRC2S) ? -1 : 1;
    FDESTS = std::abs(FSRC1S) * sign;
    FILL_DEST_HIGH;
}

void RVCore::do_fsgnjn_s(const DecodeInfo &decodeInfo) {
    int sign = std::signbit(FSRC2S) ? 1 : -1;
    FDESTS = std::abs(FSRC1S) * sign;
    FILL_DEST_HIGH;
}

void RVCore::do_fsgnjx_s(const DecodeInfo &decodeInfo) {
    int sign = std::signbit(FSRC2S) ? 1 : -1;
    FDESTS = FSRC1S * sign;
    FILL_DEST_HIGH;
}

void RVCore::do_fmin_s(const DecodeInfo &decodeInfo) {
    FDESTS = std::min(FSRC1S, FSRC2S);
    FILL_DEST_HIGH;
}

void RVCore::do_fmax_s(const DecodeInfo &decodeInfo) {
    FDESTS = std::max(FSRC1S, FSRC2S);
    FILL_DEST_HIGH;
}

void RVCore::do_feq_s(const DecodeInfo &decodeInfo) {
    DEST = FSRC1S == FSRC2S ? 1 : 0;
}

void RVCore::do_flt_s(const DecodeInfo &decodeInfo) {
    DEST = FSRC1S < FSRC2S ? 1 : 0;
}

void RVCore::do_fle_s(const DecodeInfo &decodeInfo) {
    DEST = FSRC1S <= FSRC2S ? 1 : 0;
}

static bool is_quiet_nan(float f) {
    uint32_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    return (bits & 0x7F800000) == 0x7F800000 &&
           (bits & 0x007FFFFF) != 0 &&
           (bits & 0x00400000) != 0;
}

static bool is_signaling_nan(float f) {
    uint32_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    return (bits & 0x7F800000) == 0x7F800000 &&
           (bits & 0x007FFFFF) != 0 &&
           (bits & 0x00400000) == 0;
}

void RVCore::do_fclass_s(const DecodeInfo &decodeInfo) {
    switch (std::fpclassify(FSRC1S)) {
        case FP_INFINITE:
            DEST = std::signbit(FSRC1S) ? 0 : 7;
            break;
        case FP_NORMAL:
            DEST = std::signbit(FSRC1S) ? 1 : 6;
            break;
        case FP_SUBNORMAL:
            DEST = std::signbit(FSRC1S) ? 2 : 5;
            break;
        case FP_ZERO:
            DEST = std::signbit(FSRC1S) ? 3 : 4;
            break;
        case FP_NAN:
            if (is_quiet_nan(FSRC1S)) {
                DEST = 8;
            } else if (is_signaling_nan(FSRC1S)) {
                DEST = 9;
            }
            break;
    }
}

void RVCore::do_fcvt_w_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    uint32_t i;
    if (std::isnan(FSRC1S) || FSRC1S > (float)INT32_MAX) {
        i = INT32_MAX;
    } else if (FSRC1S < INT32_MIN) {
        i = INT32_MIN;
    } else {
        i = (int32_t)FSRC1S;
    }
    DEST = (sword_t)i;
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_wu_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    if (std::isnan(FSRC1S) || FSRC1S > (float)UINT32_MAX) {
        DEST = UINT32_MAX;
    } else if (FSRC1S < 0) {
        DEST = 0;
    } else {
        DEST = (uint32_t)FSRC1S;
    }
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_s_w(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = (float)(sword_t)SRC1;
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_s_wu(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = (float)(word_t)SRC1;
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

#ifdef KXEMU_ISA64

void RVCore::do_fcvt_l_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    uint64_t i;
    if (std::isnan(FSRC1S) || FSRC1S > (float)INT64_MAX) {
        i = INT64_MAX;
    } else if (FSRC1S < INT64_MIN) {
        i = INT64_MIN;
    } else {
        i = (int64_t)FSRC1S;
    }
    DEST = (sword_t)i;
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_lu_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    if (std::isnan(FSRC1S) || FSRC1S > (float)UINT64_MAX) {
        DEST = UINT64_MAX;
    } else if (FSRC1S < 0) {
        DEST = 0;
    } else {
        DEST = (uint64_t)FSRC1S;
    }
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_s_l(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = (float)(sword_t)SRC1;
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_s_lu(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = (float)(word_t)SRC1;
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

#endif

void RVCore::do_fmv_x_w(const DecodeInfo &decodeInfo) {
    uint32_t i;
    std::memcpy(&i, &FSRC1S, 4);
    DEST = i;

    #ifdef KXEMU_ISA64
    if (std::signbit(FSRC1S)) {
        DEST |= 0xffffffff00000000;
    }
    #endif
}

void RVCore::do_fmv_w_x(const DecodeInfo &decodeInfo) {
    uint32_t i = SRC1;
    float f;
    std::memcpy(&f, &i, 4);
    FDESTS = f;
    FILL_DEST_HIGH;
}

void RVCore::do_fld(const DecodeInfo &decodeInfo) {
    uint64_t i = this->memory_load(SRC1 + IMM, 8);
    double d;
    std::memcpy(&d, &i, 8);
    FDESTD = d;
}

void RVCore::do_fsd(const DecodeInfo &decodeInfo) {
    uint64_t i;
    std::memcpy(&i, &FSRC2D, 8);
    this->memory_store(SRC1 + IMM, i, 8);
}

void RVCore::do_fmadd_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = FSRC1D * FSRC2D + FSRC3D;
    
    RESTORE_FPU;
}

void RVCore::do_fmsub_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = FSRC1D * FSRC2D - FSRC3D;
    
    RESTORE_FPU;
}

void RVCore::do_fnmsub_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = -FSRC1D * FSRC2D + FSRC3D;
    
    RESTORE_FPU;
}

void RVCore::do_fnmadd_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = -FSRC1D * FSRC2D - FSRC3D;
    
    RESTORE_FPU;
}

void RVCore::do_fadd_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = FSRC1D + FSRC2D;
    
    RESTORE_FPU;
}

void RVCore::do_fsub_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = FSRC1D - FSRC2D;
    
    RESTORE_FPU;
}

void RVCore::do_fmul_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = FSRC1D * FSRC2D;
    
    RESTORE_FPU;
}

void RVCore::do_fdiv_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = FSRC1D / FSRC2D;
    
    RESTORE_FPU;
}

void RVCore::do_fsqrt_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = std::sqrt(FSRC1D);
    
    RESTORE_FPU;
}

void RVCore::do_fsgnj_d(const DecodeInfo &decodeInfo) {
    int sign = std::signbit(FSRC2D) ? -1 : 1;
    FDESTD = std::abs(FSRC1D) * sign;
}

void RVCore::do_fsgnjn_d(const DecodeInfo &decodeInfo) {
    int sign = std::signbit(FSRC2D) ? 1 : -1;
    FDESTD = std::abs(FSRC1D) * sign;
}

void RVCore::do_fsgnjx_d(const DecodeInfo &decodeInfo) {
    int sign = std::signbit(FSRC2D) ? 1 : -1;
    FDESTD = FSRC1D * sign;
}

void RVCore::do_fmin_d(const DecodeInfo &decodeInfo) {
    FDESTD = std::min(FSRC1D, FSRC2D);
}

void RVCore::do_fmax_d(const DecodeInfo &decodeInfo) {
    FDESTD = std::max(FSRC1D, FSRC2D);
}

void RVCore::do_feq_d(const DecodeInfo &decodeInfo) {
    DEST = FSRC1D == FSRC2D ? 1 : 0;
}

void RVCore::do_flt_d(const DecodeInfo &decodeInfo) {
    DEST = FSRC1D < FSRC2D ? 1 : 0;
}

void RVCore::do_fle_d(const DecodeInfo &decodeInfo) {
    DEST = FSRC1D <= FSRC2D ? 1 : 0;
}

static bool is_quiet_nan(double f) {
    uint64_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    return (bits & 0x7ff0000000000000) == 0x7ff0000000000000 &&
           (bits & 0x000fffffffffffff) != 0 &&
           (bits & 0x0008000000000000) != 0;
}

static bool is_signaling_nan(double f) {
    uint64_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    return (bits & 0x7ff0000000000000) == 0x7ff0000000000000 &&
           (bits & 0x000fffffffffffff) != 0 &&
           (bits & 0x0008000000000000) == 0;
}

void RVCore::do_fclass_d(const DecodeInfo &decodeInfo) {
    switch (std::fpclassify(FSRC1D)) {
        case FP_INFINITE:
            DEST = std::signbit(FSRC1D) ? 0 : 7;
            break;
        case FP_NORMAL:
            DEST = std::signbit(FSRC1D) ? 1 : 6;
            break;
        case FP_SUBNORMAL:
            DEST = std::signbit(FSRC1D) ? 2 : 5;
            break;
        case FP_ZERO:
            DEST = std::signbit(FSRC1D) ? 3 : 4;
            break;
        case FP_NAN:
            if (is_quiet_nan(FSRC1D)) {
                DEST = 8;
            } else if (is_signaling_nan(FSRC1D)) {
                DEST = 9;
            }
            break;
    }
}

void RVCore::do_fcvt_s_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTS = (float)FSRC1D;
    FILL_DEST_HIGH;
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_d_s(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = (double)FSRC1S;
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_w_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    uint32_t i;
    if (std::isnan(FSRC1D) || FSRC1D > INT32_MAX) {
        i = INT32_MAX;
    } else if (FSRC1D < INT32_MIN) {
        i = INT32_MIN;
    } else {
        i = (int32_t)FSRC1D;
    }
    DEST = (sword_t)i;
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_wu_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    if (std::isnan(FSRC1D) || FSRC1D > UINT32_MAX) {
        DEST = UINT32_MAX;
    } else if (FSRC1D < 0) {
        DEST = 0;
    } else {
        DEST = (uint32_t)FSRC1D;
    }
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_d_w(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = (double)(sword_t)SRC1;
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_d_wu(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = (double)(word_t)SRC1;
    
    RESTORE_FPU;
}

#ifdef KXEMU_ISA64

void RVCore::do_fcvt_l_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    uint64_t i;
    if (std::isnan(FSRC1D) || FSRC1D > (double)INT64_MAX) {
        i = INT64_MAX;
    } else if (FSRC1D < INT64_MIN) {
        i = INT64_MIN;
    } else {
        i = (int64_t)FSRC1D;
    }
    DEST = (sword_t)i;
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_lu_d(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    if (std::isnan(FSRC1D) || FSRC1D > (double)UINT64_MAX) {
        DEST = UINT64_MAX;
    } else if (FSRC1D < 0) {
        DEST = 0;
    } else {
        DEST = (uint64_t)FSRC1D;
    }
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_d_l(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = (double)(sword_t)SRC1;
    
    RESTORE_FPU;
}

void RVCore::do_fcvt_d_lu(const DecodeInfo &decodeInfo) {
    SET_FPU;
    
    FDESTD = (double)(word_t)SRC1;
    
    RESTORE_FPU;
}

void RVCore::do_fmv_x_d(const DecodeInfo &decodeInfo) {
    uint64_t i;
    std::memcpy(&i, &FSRC1D, 8);
    DEST = i;

    #ifdef KXEMU_ISA64
    if (std::signbit(FSRC1D)) {
        DEST |= 0xffffffff00000000;
    }
    #endif
}

void RVCore::do_fmv_d_x(const DecodeInfo &decodeInfo) {
    uint64_t i = SRC1;
    double d;
    std::memcpy(&d, &i, 8);
    FDESTD = d;
}

#endif
