/*
 * Floating-point disabled (CONFIG_FPU_NONE).
 *
 * Without the F/D extensions every floating-point encoding, including
 * loads/stores and CSR-independent moves, is an illegal instruction.
 */
#include "cpu/riscv/core.hpp"

#include "config/config.h"

using namespace kxemu::cpu;

#ifdef CONFIG_FPU_NONE

void RVCore::do_flw(const DecodeInfo &decodeInfo)    { this->do_invalid_inst(); }
void RVCore::do_fsw(const DecodeInfo &decodeInfo)    { this->do_invalid_inst(); }
void RVCore::do_fld(const DecodeInfo &decodeInfo)    { this->do_invalid_inst(); }
void RVCore::do_fsd(const DecodeInfo &decodeInfo)    { this->do_invalid_inst(); }

void RVCore::do_fmadd_s  (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fmsub_s  (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fnmsub_s (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fnmadd_s (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fadd_s   (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fsub_s   (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fmul_s   (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fdiv_s   (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fsqrt_s  (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fsgnj_s  (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fsgnjn_s (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fsgnjx_s (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fmin_s   (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fmax_s   (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_feq_s    (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_flt_s    (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fle_s    (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fclass_s (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_w_s (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_wu_s(const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_s_w (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_s_wu(const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fmv_x_w  (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fmv_w_x  (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }

void RVCore::do_fmadd_d  (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fmsub_d  (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fnmsub_d (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fnmadd_d (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fadd_d   (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fsub_d   (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fmul_d   (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fdiv_d   (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fsqrt_d  (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fsgnj_d  (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fsgnjn_d (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fsgnjx_d (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fmin_d   (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fmax_d   (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_feq_d    (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_flt_d    (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fle_d    (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fclass_d (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_s_d (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_d_s (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_w_d (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_wu_d(const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_d_w (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_d_wu(const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }

#ifdef KXEMU_ISA64

void RVCore::do_fcvt_l_s (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_lu_s(const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_s_l (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_s_lu(const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fmv_x_d  (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fmv_d_x  (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_l_d (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_lu_d(const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_d_l (const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }
void RVCore::do_fcvt_d_lu(const DecodeInfo &decodeInfo) { this->do_invalid_inst(); }

#endif // KXEMU_ISA64

#endif // CONFIG_FPU_NONE
