#define BITS(hi, lo) ((uint32_t)((this->inst >> lo) & ((1 << (hi - lo + 1)) - 1))) // Extract bits from hi to lo
#define SEXT(bits, from) ((int32_t)((bits) << (32 - (from))) >> (32 - (from))) // Signed extend
