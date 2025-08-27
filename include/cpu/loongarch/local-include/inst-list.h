#include "config/config.h"

#define _INST(name) void do_##name();

_INST(add_w)
_INST(sub_w)
_INST(slt)
_INST(sltu)
_INST(maskeqz)
_INST(masknez)
_INST(nor)
_INST(and)
_INST(or)
_INST(xor)
_INST(orn)
_INST(andn)
_INST(sll_w)
_INST(srl_w)
_INST(sra_w)

#ifdef KXEMU_ISA64
_INST(add_d)
_INST(sub_d)
#endif

_INST(addi_w)
_INST(slti)
_INST(sltiu)
_INST(ori)
_INST(xori)
_INST(andi)
_INST(slli_w)
_INST(srli_w)
_INST(srai_w)

_INST(mul_w)
_INST(mulh_w)
_INST(mulh_wu)
_INST(div_w)
_INST(div_wu)
_INST(mod_w)
_INST(mod_wu)

_INST(beq)
_INST(bne)
_INST(blt)
_INST(bltu)
_INST(bge)
_INST(bgeu)
_INST(b)
_INST(bl)
_INST(jirl)

_INST(lu12i_w)
_INST(pcaddu12i)

_INST(ld_b)
_INST(ld_h)
_INST(ld_w)
_INST(ld_bu)
_INST(ld_hu)
_INST(st_b)
_INST(st_h)
_INST(st_w)

_INST(break)

#undef _INST
