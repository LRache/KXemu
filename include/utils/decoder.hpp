#ifndef __KXEMU_UTILS_DECODER_HPP__
#define __KXEMU_UTILS_DECODER_HPP__

#include "log.h"
#include "macro.h"

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
    BitPat(const std::string &s) {
        uint64_t bits = 0;
        uint64_t mask = 0;
        unsigned int length = 0;
        for (int i = s.length() - 1; i >= 0; i--) {
            switch (s[i]) {
                case '0': length++; break;
                case '1': bits |= 1 << length; length++; break;
                case 'x': mask |= 1 << length; length++; break;
                case '?': mask |= 1 << length; length++; break;
                case '_': break;
                case ' ': break;
                default: PANIC("Invalid BitPat %s", s.c_str());
            }
        }
        if (length > 64) {
            PANIC("BitPat length %d is too long", length);
        }
        
        for (std::size_t i = length; i < 64; i++) {
            mask |= 1UL << i;
        }

        this->bits = bits;
        this->mask = ~mask; // mask is 0 for match any bit
        this->length = length;
    }
    
    BitPat(BitPat &other) {
        this->bits = other.bits;
        this->mask = other.mask;
        this->length = other.length;
    }

    BitPat(BitPat &&other) {
        this->bits = other.bits;
        this->mask = other.mask;
        this->length = other.length;
    }

    unsigned int get_length() const {
        return this->length;
    }
    
    bool match(uint64_t data) const {
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

    unsigned int count() const {
        return patterns.size();
    }
};

} // namespace kxemu::utils

#endif