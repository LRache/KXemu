#include "config/config.h"

#define _INST(name) void do_##name(const DecodeInfo &);

_INST(add)
_INST(sub)
_INST(and)
_INST(or)
_INST(xor)
_INST(sll)
_INST(srl)
_INST(sra)
_INST(slt)
_INST(sltu)

_INST(addi)
_INST(andi)
_INST(ori)
_INST(xori)
_INST(slli)
_INST(srli)
_INST(srai)
_INST(slti)
_INST(sltiu)

_INST(lb)
_INST(lbu)
_INST(lh)
_INST(lhu)
_INST(lw)

_INST(sb)
_INST(sh)
_INST(sw)

_INST(beq)
_INST(bge)
_INST(bgeu)
_INST(blt)
_INST(bltu)
_INST(bne)

_INST(jal)
_INST(jalr)

_INST(lui)
_INST(auipc)

// RV64 only
#ifdef KXEMU_ISA64
_INST(addw)
_INST(subw)
_INST(sllw)
_INST(srlw)
_INST(sraw)

_INST(addiw)
_INST(slliw)
_INST(srliw)
_INST(sraiw)

_INST(lwu)
_INST(ld)

_INST(sd)
#endif

// Integer multiplication and division extension
_INST(mul)
_INST(mulh)
_INST(mulhsu)
_INST(mulhu)
_INST(div)
_INST(divu)
_INST(rem)
_INST(remu)

// RV64 only
#ifdef KXEMU_ISA64
_INST(mulw)
_INST(divw)
_INST(divuw)
_INST(remw)
_INST(remuw)
#endif

// Atomic extension
_INST(lr_w)
_INST(sc_w)
_INST(amoswap_w)
_INST(amoadd_w)
_INST(amoxor_w)
_INST(amoand_w)
_INST(amoor_w)
_INST(amomin_w)
_INST(amomax_w)
_INST(amominu_w)
_INST(amomaxu_w)
#ifdef KXEMU_ISA64
_INST(lr_d)
_INST(sc_d)
_INST(amoswap_d)
_INST(amoadd_d)
_INST(amoxor_d)
_INST(amoand_d)
_INST(amoor_d)
_INST(amomin_d)
_INST(amomax_d)
_INST(amominu_d)
_INST(amomaxu_d)
#endif

_INST(flw)
_INST(fsw)
_INST(fmadd_s)
_INST(fmsub_s)
_INST(fnmsub_s)
_INST(fnmadd_s)
_INST(fadd_s)
_INST(fsub_s)
_INST(fmul_s)
_INST(fdiv_s)
_INST(fsqrt_s)
_INST(fsgnj_s)
_INST(fsgnjn_s)
_INST(fsgnjx_s)
_INST(fmin_s)
_INST(fmax_s)
_INST(feq_s)
_INST(flt_s)
_INST(fle_s)
_INST(fclass_s)
_INST(fcvt_w_s)
_INST(fcvt_wu_s)
_INST(fcvt_s_w)
_INST(fcvt_s_wu)
_INST(fmv_x_w)
_INST(fmv_w_x)

#ifdef KXEMU_ISA64
_INST(fcvt_l_s)
_INST(fcvt_lu_s)
_INST(fcvt_s_l)
_INST(fcvt_s_lu)
#endif

_INST(fld)
_INST(fsd)
_INST(fmadd_d)
_INST(fmsub_d)
_INST(fnmsub_d)
_INST(fnmadd_d)
_INST(fadd_d)
_INST(fsub_d)
_INST(fmul_d)
_INST(fdiv_d)
_INST(fsqrt_d)
_INST(fsgnj_d)
_INST(fsgnjn_d)
_INST(fsgnjx_d)
_INST(fmin_d)
_INST(fmax_d)
_INST(feq_d)
_INST(flt_d)
_INST(fle_d)
_INST(fclass_d)
_INST(fcvt_s_d)
_INST(fcvt_d_s)
_INST(fcvt_w_d)
_INST(fcvt_wu_d)
_INST(fcvt_d_w)
_INST(fcvt_d_wu)

#ifdef KXEMU_ISA64
_INST(fcvt_l_d)
_INST(fcvt_lu_d)
_INST(fcvt_d_l)
_INST(fcvt_d_lu)
_INST(fmv_x_d)
_INST(fmv_d_x)
#endif

// Compressed extension
_INST(c_lwsp)
_INST(c_swsp)

_INST(c_lw)
_INST(c_sw)

_INST(c_j)
_INST(c_jal)
_INST(c_jr)
_INST(c_jalr)

_INST(c_beqz)
_INST(c_bnez)

_INST(c_li)
_INST(c_lui)

_INST(c_addi)
_INST(c_addi16sp)
_INST(c_addi4spn)

_INST(c_slli)
_INST(c_srli)
_INST(c_srai)
_INST(c_andi)

_INST(c_mv)
_INST(c_add)
_INST(c_sub)
_INST(c_xor)
_INST(c_or)
_INST(c_and)
_INST(c_nop)

#ifdef KXEMU_ISA64
_INST(c_ldsp)
_INST(c_sdsp)

_INST(c_ld)
_INST(c_sd)

_INST(c_fldsp)
_INST(c_fsdsp)
_INST(c_fld)
_INST(c_fsd)

_INST(c_addiw)
_INST(c_addw)
_INST(c_subw)
#else
_INST(c_flwsp)
_INST(c_fswsp)

_INST(c_flw)
_INST(c_fsw)
#endif

// Privileged mode
_INST(ecall)
_INST(mret)
_INST(sret)
_INST(ebreak)
_INST(wfi)

_INST(sfence_vma)
_INST(fence)

// Zifenci extenstion
_INST(fence_i)

// Zicsr extension
_INST(csrrw)
_INST(csrrs)
_INST(csrrc)
_INST(csrrwi)
_INST(csrrsi)
_INST(csrrci)

#undef _INST
