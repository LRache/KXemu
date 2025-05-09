#ifndef __KXEMU_CPU_WORD_H__
#define __KXEMU_CPU_WORD_H__

#include "config/config.h"

#include <cstdint>

namespace kxemu::cpu {
#ifdef KXEMU_ISA64
    using  word_t = uint64_t;
    using sword_t =  int64_t;
    using  dword_t = unsigned __int128;
    using sdword_t = __int128;
#else
    using  word_t = uint32_t;
    using sword_t =  int32_t;
    using  dword_t = uint64_t;
    using sdword_t =  int64_t;
#endif

    static constexpr inline unsigned int WORD_BITS = sizeof(word_t) * 8;
}

#endif
