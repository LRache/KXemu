#ifndef __COMMON_H__
#define __COMMON_H__

#include <cinttypes>

#define FMT_WORD_64 "0x%016" PRIx64
#define FMT_WORD_32 "0x%08" PRIx32

#ifdef ISA_64
    #define FMT_WORD FMT_WORD_64
    using word_t = uint64_t;
#else
    #define FMT_WORD FMT_WORD_32
    using word_t = uint32_t;
#endif

#endif
