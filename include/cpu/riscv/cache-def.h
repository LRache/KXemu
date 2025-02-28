#ifndef __KXEMU_CPU_RISCV_ICACHE_DEF_H__
#define __KXEMU_CPU_RISCV_ICACHE_DEF_H__

// #define ICACHE_SET_BITS 11
#define ICACHE_TAG_BITS (WORD_BITS - ICACHE_SET_BITS - 1)
#define ICACHE_SET_MASK ((word_t)((1ULL << ICACHE_SET_BITS) - 1) << 1)
#define ICACHE_TAG_MASK ((word_t)(((1ULL << ICACHE_TAG_BITS) - 1)) << (ICACHE_SET_BITS + 1))
#define ICACHE_SET(pc) (((pc) & ICACHE_SET_MASK) >> 1)
#define ICACHE_TAG(pc)  ((pc) & ICACHE_TAG_MASK)

#define DCACHE_TAG_BITS (WORD_BITS - DCACHE_BLOCK_BITS - DCACHE_SET_BITS)
#define DCACHE_OFF_BITS DCACHE_BLOCK_BITS
#define DCACHE_OFF_MASK ((word_t)((1ULL << DCACHE_OFF_BITS) - 1))
#define DCACHE_SET_MASK ((word_t)((1ULL << DCACHE_SET_BITS) - 1) << DCACHE_OFF_BITS)
#define DCACHE_TAG_MASK ((word_t)(((word_t)-1) & ~DCACHE_OFF_MASK & ~DCACHE_SET_MASK))
#define DCACHE_OFF(addr)  ((addr) & DCACHE_OFF_MASK)
#define DCACHE_SET(addr) (((addr) & DCACHE_SET_MASK) >> DCACHE_OFF_BITS)
#define DCACHE_TAG(addr)  ((addr) & DCACHE_TAG_MASK)

#endif
