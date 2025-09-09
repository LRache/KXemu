#ifndef __KXEMU_CPU_RISCV_CONFIG_HPP__
#define __KXEMU_CPU_RISCV_CONFIG_HPP__

#include "config/config.h"

namespace kxemu::cpu::config {
    constexpr inline unsigned int ICACHE_SET_BITS = CONFIG_ICACHE_SET_BITS;
    constexpr inline unsigned int TLB_SET_BITS = CONFIG_TLB_SET_BITS;
}

#endif // __KXEMU_CPU_RISCV_CONFIG_HPP__
