/*
 * Floating-point arithmetic on the host FPU (CONFIG_FPU_HOST).
 *
 * Note: RMM (round to nearest, ties away from zero) has no host fenv
 * counterpart and is approximated by FE_TONEAREST. Integer conversions
 * truncate toward zero instead of following the guest rounding mode.
 */
#include "cpu/riscv/core.hpp"
#include "cpu/riscv/csr-field.hpp"
#include "cpu/word.hpp"

#include <cfenv>
#include <cstdint>
#include <cmath>
#include <algorithm>

#include "./local-decoder.h"

using namespace kxemu::cpu;

#ifdef CONFIG_FPU_HOST

// RISC-V fflags bits
#define FFLAGS_NV 0x10 // Invalid
#define FFLAGS_DZ 0x08 // Divide by zero
#define FFLAGS_OF 0x04 // Overflow
#define FFLAGS_UF 0x02 // Underflow
#define FFLAGS_NX 0x01 // Inexact

static void set_fpu_rounding_mode(unsigned int rm) {
    switch (rm) {
        case csr::FCSR::RNE:
            fesetround(FE_TONEAREST);
            break;
        case csr::FCSR::RTZ:
            fesetround(FE_TOWARDZERO);
            break;
        case csr::FCSR::RDN:
            fesetround(FE_DOWNWARD);
            break;
        case csr::FCSR::RUP:
            fesetround(FE_UPWARD);
            break;
        case csr::FCSR::RMM:
            fesetround(FE_TONEAREST);
            break;
        case csr::FCSR::DYN:
            break;
    }
}

#define SET_FPU \
    unsigned int rm = this->frm; \
    if (rm == csr::FCSR::DYN) { \
        rm = decodeInfo.flag; \
        if (unlikely(rm == 5 || rm == 6)) { \
            this->do_invalid_inst(); \
            return; \
        } \
    } \
    int old = fegetround(); \
    set_fpu_rounding_mode(rm);

#define COLLECT_FENV_FFLAGS \
    do { \
        int excepts = fetestexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW | FE_INEXACT); \
        if (unlikely(excepts != 0)) { \
            word_t flags = 0; \
            if (excepts & FE_INVALID)   flags |= FFLAGS_NV; \
            if (excepts & FE_DIVBYZERO) flags |= FFLAGS_DZ; \
            if (excepts & FE_OVERFLOW)  flags |= FFLAGS_OF; \
            if (excepts & FE_UNDERFLOW) flags |= FFLAGS_UF; \
            if (excepts & FE_INEXACT)   flags |= FFLAGS_NX; \
            this->raise_fflags(flags); \
            feclearexcept(FE_ALL_EXCEPT); \
        } \
    } while (0)

#define RESTORE_FPU do { COLLECT_FENV_FFLAGS; fesetround(old); } while (0)

void RVCore::do_fmadd_s(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = FSRC1S * FSRC2S + FSRC3S;
    FPR_FILL_DEST_HIGH;

    RESTORE_FPU;
}

void RVCore::do_fmsub_s(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = FSRC1S * FSRC2S - FSRC3S;
    FPR_FILL_DEST_HIGH;

    RESTORE_FPU;
}

void RVCore::do_fnmsub_s(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = -FSRC1S * FSRC2S + FSRC3S;
    FPR_FILL_DEST_HIGH;

    RESTORE_FPU;
}

void RVCore::do_fnmadd_s(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = -FSRC1S * FSRC2S - FSRC3S;
    FPR_FILL_DEST_HIGH;

    RESTORE_FPU;
}

void RVCore::do_fadd_s(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = FSRC1S + FSRC2S;
    FPR_FILL_DEST_HIGH;

    RESTORE_FPU;
}

void RVCore::do_fsub_s(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = FSRC1S - FSRC2S;
    FPR_FILL_DEST_HIGH;

    RESTORE_FPU;
}

void RVCore::do_fmul_s(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = FSRC1S * FSRC2S;
    FPR_FILL_DEST_HIGH;

    RESTORE_FPU;
}

void RVCore::do_fdiv_s(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = FSRC1S / FSRC2S;
    FPR_FILL_DEST_HIGH;

    RESTORE_FPU;
}

void RVCore::do_fsqrt_s(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = std::sqrt(FSRC1S);
    FPR_FILL_DEST_HIGH;

    RESTORE_FPU;
}

void RVCore::do_fmin_s(const DecodeInfo &decodeInfo) {
    FDESTS = std::min(FSRC1S, FSRC2S);
    FPR_FILL_DEST_HIGH;
}

void RVCore::do_fmax_s(const DecodeInfo &decodeInfo) {
    FDESTS = std::max(FSRC1S, FSRC2S);
    FPR_FILL_DEST_HIGH;
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

void RVCore::do_fcvt_w_s(const DecodeInfo &decodeInfo) {
    SET_FPU;

    uint32_t i;
    if (std::isnan(FSRC1S) || FSRC1S > (float)INT32_MAX) {
        this->raise_fflags(FFLAGS_NV);
        i = INT32_MAX;
    } else if (FSRC1S < INT32_MIN) {
        this->raise_fflags(FFLAGS_NV);
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
        this->raise_fflags(FFLAGS_NV);
        DEST = UINT32_MAX;
    } else if (FSRC1S < 0) {
        this->raise_fflags(FFLAGS_NV);
        DEST = 0;
    } else {
        DEST = (uint32_t)FSRC1S;
    }

    RESTORE_FPU;
}

void RVCore::do_fcvt_s_w(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = (float)(sword_t)SRC1;
    FPR_FILL_DEST_HIGH;

    RESTORE_FPU;
}

void RVCore::do_fcvt_s_wu(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = (float)(word_t)SRC1;
    FPR_FILL_DEST_HIGH;

    RESTORE_FPU;
}

#ifdef KXEMU_ISA64

void RVCore::do_fcvt_l_s(const DecodeInfo &decodeInfo) {
    SET_FPU;

    uint64_t i;
    if (std::isnan(FSRC1S) || FSRC1S > (float)INT64_MAX) {
        this->raise_fflags(FFLAGS_NV);
        i = INT64_MAX;
    } else if (FSRC1S < INT64_MIN) {
        this->raise_fflags(FFLAGS_NV);
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
        this->raise_fflags(FFLAGS_NV);
        DEST = UINT64_MAX;
    } else if (FSRC1S < 0) {
        this->raise_fflags(FFLAGS_NV);
        DEST = 0;
    } else {
        DEST = (uint64_t)FSRC1S;
    }

    RESTORE_FPU;
}

void RVCore::do_fcvt_s_l(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = (float)(sword_t)SRC1;
    FPR_FILL_DEST_HIGH;

    RESTORE_FPU;
}

void RVCore::do_fcvt_s_lu(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = (float)(word_t)SRC1;
    FPR_FILL_DEST_HIGH;

    RESTORE_FPU;
}

#endif // KXEMU_ISA64

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
    if (std::isnan(FSRC1D) || std::isnan(FSRC2D)) {
        DEST = 0;
    } else {
        DEST = FSRC1D < FSRC2D ? 1 : 0;
    }
}

void RVCore::do_fle_d(const DecodeInfo &decodeInfo) {
    if (std::isnan(FSRC1D) || std::isnan(FSRC2D)) {
        DEST = 0;
    } else {
        DEST = FSRC1D <= FSRC2D ? 1 : 0;
    }
}

void RVCore::do_fcvt_s_d(const DecodeInfo &decodeInfo) {
    SET_FPU;

    FDESTS = (float)FSRC1D;
    FPR_FILL_DEST_HIGH;

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
        this->raise_fflags(FFLAGS_NV);
        i = INT32_MAX;
    } else if (FSRC1D < INT32_MIN) {
        this->raise_fflags(FFLAGS_NV);
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
        this->raise_fflags(FFLAGS_NV);
        DEST = UINT32_MAX;
    } else if (FSRC1D < 0) {
        this->raise_fflags(FFLAGS_NV);
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
    if (std::isnan(FSRC1D)) {
        this->raise_fflags(FFLAGS_NV);
        i = INT64_MAX;
    } else if (FSRC1D > (double)INT64_MAX) {
        this->raise_fflags(FFLAGS_NV);
        i = INT64_MAX;
    } else if (FSRC1D < (double)INT64_MIN) {
        this->raise_fflags(FFLAGS_NV);
        i = INT64_MIN;
    } else {
        // Round according to the current rounding mode
        i = (int64_t)llrint(FSRC1D);
    }
    DEST = (sword_t)i;

    RESTORE_FPU;
}

void RVCore::do_fcvt_lu_d(const DecodeInfo &decodeInfo) {
    SET_FPU;

    if (std::isnan(FSRC1D) || FSRC1D > (double)UINT64_MAX) {
        this->raise_fflags(FFLAGS_NV);
        DEST = UINT64_MAX;
    } else if (FSRC1D < 0) {
        this->raise_fflags(FFLAGS_NV);
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

#endif // KXEMU_ISA64

#endif // CONFIG_FPU_HOST
