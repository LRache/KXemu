#include "cpu/riscv/core.h"

using namespace kxemu::cpu;

bool RVCore::decode_and_exec() {
    uint32_t inst = this->inst;
    #include "../autogen/base-decoder.h"
}

bool RVCore::decode_and_exec_c() {
    uint32_t inst = this->inst & 0xffff;
    #include "../autogen/compressed-decoder.h"
}
