#include "cpu/decoder.h"
#include "log.h"
#include <cstdint>

BitPat::BitPat(std::string s) {
    uint64_t bits = 0;
    uint64_t mask = 0;
    int length = 0;
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
    this->bits = bits;
    this->mask = ~mask; // mask is 0 for match any bit
    this->length = length;
}

BitPat::BitPat(BitPat &other) {
    this->bits = other.bits;
    this->mask = other.mask;
    this->length = other.length;
}

BitPat::BitPat(BitPat &&other) {
    this->bits = other.bits;
    this->mask = other.mask;
    this->length = other.length;
}

int BitPat::get_length() {
    return this->length;
}

bool BitPat::match(uint64_t data) {
    // INFO("data: 0x%lx, bits: 0x%lx, mask: 0x%lx", data, this->bits, this->mask);
    return (data & this->mask) == this->bits;
}
