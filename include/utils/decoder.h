#ifndef __KXEMU_UTILS_DECODER_H__
#define __KXEMU_UTILS_DECODER_H__

#include "log.h"
#include "macro.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace kxemu::utils {

class BitPat {
private:
    uint64_t bits;
    uint64_t mask;
    unsigned int length;
public:
    BitPat(const std::string &s);
    BitPat(BitPat &other);
    BitPat(BitPat &&other);
    unsigned int get_length() const;
    bool match(uint64_t data) const { // To inline this function
        return (data & this->mask) == this->bits;
    }
};

template<typename T>
class Decoder {
private:
    struct Entry {
        BitPat pattern;
        void (T::*action)();
    };
    std::vector<Entry> patterns;
    // Entry patterns[N];
    T *obj;
public:
    unsigned int fixedLength = 0;
    
    void init(T *obj) {
        this->obj = obj;
    }

    void add(const std::string &pattern, void (T::*action)()) {
        if (patterns.empty()) {
            fixedLength = BitPat(pattern).get_length();
        } else {
            if (fixedLength != BitPat(pattern).get_length()) {
                PANIC("Pattern length mismatch");
            }
        }
        patterns.push_back({BitPat(pattern), action});
    }

    bool decode_and_exec(uint64_t bits) const {
        for (const Entry &p : patterns) {
            if (unlikely(p.pattern.match(bits))) {
                (this->obj->*(p.action))();
                return true;
            }
        }
        return false;
    }
};

} // namespace kxemu::utils

#endif