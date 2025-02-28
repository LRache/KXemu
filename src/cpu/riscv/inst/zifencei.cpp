#include "cpu/riscv/core.h"

using namespace kxemu::cpu;

void RVCore::do_fence_i(const DecodeInfo &) {
    for (unsigned int i = 0; i < sizeof(this->icache) / sizeof(this->icache[0]); i++) {
        this->icache[i].valid = false;
    }
}
