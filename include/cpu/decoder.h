#ifndef __CPU_DECODER_H__
#define __CPU_DECODER_H__

#include <cstdint>
#include <string>
#include <vector>

class BitPat {
private:
    uint64_t bits;
    uint64_t mask;
    int length;
public:
    BitPat(std::string s);
    BitPat(BitPat &other);
    BitPat(BitPat &&other);
    int get_length();
    bool match(uint64_t data);
};

template<typename T>
class Decoder {
private:
    std::vector<BitPat> patterns;
    std::vector<void (T::*)()> actions;
    T *obj;
public:
    void init(T *obj) {
        this->obj = obj;
    }

    void add(std::string pattern, void (T::*action)()) {
        patterns.push_back(BitPat(pattern));
        actions.push_back(action);
    }

    bool decode_and_exec(uint64_t bits) {
        for (size_t i = 0; i < patterns.size(); i++) {
            if (patterns[i].match(bits)) {
                (this->obj->*actions[i])();
                return true;
            }
        }
        return false;
    }
};

#endif
