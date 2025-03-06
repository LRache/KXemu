#define _DECODE(name) void decode_insttype_##name();

_DECODE(r)
_DECODE(i)
_DECODE(shifti)
_DECODE(shiftiw)
_DECODE(s)
_DECODE(b)
_DECODE(lui)
_DECODE(auipc)
_DECODE(j)
_DECODE(csrr)
_DECODE(csri)

_DECODE(c_lwsp)
_DECODE(c_swsp)
_DECODE(c_lwsw)
_DECODE(c_j)
_DECODE(c_jalr)
_DECODE(c_b)
_DECODE(c_li)
_DECODE(c_lui)
_DECODE(c_addi)
_DECODE(c_addi16sp)
_DECODE(c_addi4spn)
_DECODE(c_slli)
_DECODE(c_i)
_DECODE(c_shifti)
_DECODE(c_mvadd)
_DECODE(c_r)

#ifdef KXEMU_ISA64
_DECODE(c_ldsp)
_DECODE(c_sdsp)
_DECODE(c_ldsd)
#endif

_DECODE(n)

#undef _DECODE
