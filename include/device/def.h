#ifndef __KXEMU_DEVICE_DEF_H__
#define __KXEMU_DEVICE_DEF_H__

#include <cstdint>

namespace kxemu::device {
    
    using word_t = uint64_t;

    struct AddrSpace {
        const word_t BASE;
        const word_t SIZE;
        
        bool in_range(word_t addr) const {
            return (addr >= BASE && addr < BASE + SIZE);
        }
    };

    enum class AMO {
        SWAP,
        ADD,
        AND,
        OR,
        XOR,
        MIN,
        MAX,
        MINU,
        MAXU
    };
}

#endif
