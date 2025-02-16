#ifndef __KXEMU_DEVICE_DEF_H__
#define __KXEMU_DEVICE_DEF_H__

#include <cstdint>

namespace kxemu::device {
    
    using word_t = uint64_t;

    enum AMO {
        AMO_SWAP,
        AMO_ADD,
        AMO_AND,
        AMO_OR,
        AMO_XOR,
        AMO_MIN,
        AMO_MAX,
        AMO_MINU,
        AMO_MAXU
    };
}

#endif
