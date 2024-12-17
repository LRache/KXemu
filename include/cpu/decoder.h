#ifndef __CPU_DECODER_H__
#define __CPU_DECODER_H__

#include "log.h"
#include "macro.h"
#include <cstdint>
#include <string>
#include <vector>

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
    bool match(uint64_t data) const;
};

template<typename T>
class Decoder {
private:
    std::vector<BitPat> patterns;
    std::vector<void (T::*)()> actions;
    T *obj;
public:
    unsigned int fixedLength = 0;
    
    void init(T *obj) {
        this->obj = obj;
    }

    void add(std::string pattern, void (T::*action)()) {
        if (patterns.empty()) {
            fixedLength = BitPat(pattern).get_length();
        } else {
            if (fixedLength != BitPat(pattern).get_length()) {
                PANIC("Pattern length mismatch");
            }
        }
        patterns.push_back(BitPat(pattern));
        actions.push_back(action);
    }

    bool decode_and_exec(uint64_t bits) const {
        for (size_t i = 0; i < patterns.size(); i++) {
            if (unlikely(patterns[i].match(bits))) {
                (this->obj->*actions[i])();
                return true;
            }
        }
        return false;
    }
};

#endif
