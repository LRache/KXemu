#include "cpu/riscv/core.h"

using namespace kxemu::cpu;

void RVCore::do_sfence_vma(const DecodeInfo &) {
    if (this->privMode == PrivMode::USER) {
        do_invalid_inst();
        return;
    }
    this->icache_fence();
}

void RVCore::do_fence(const DecodeInfo &) {
    if (this->privMode == PrivMode::USER) {
        do_invalid_inst();
        return;
    } 
}

void RVCore::do_fence_i(const DecodeInfo &) {
    this->icache_fence();
}
