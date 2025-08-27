#ifndef __KXEMU_CPU_RISCV_ADDR_H__
#define __KXEMU_CPU_RISCV_ADDR_H__

#include "cpu/word.h"
#include "cpu/riscv/def.h"
#include "cpu/riscv/config.hpp"

namespace kxemu::cpu {
    
    class Addr {
    private:
        word_t addr;

        static constexpr word_t ICACHE_SET_MASK = ((1ULL << config::ICACHE_SET_BITS) - 1) << 1;
        static constexpr word_t ICACHE_TAG_BITS = WORD_BITS - config::ICACHE_SET_BITS - 1;
        static constexpr word_t ICACHE_TAG_MASK = ((1ULL << ICACHE_TAG_BITS) - 1) << (config::ICACHE_SET_BITS + 1);

        static constexpr word_t TLB_TAG_BITS = WORD_BITS - config::TLB_SET_BITS - PGBITS;
        static constexpr word_t TLB_OFF_MASK = (1ULL << PGBITS) - 1;
        static constexpr word_t TLB_SET_MASK = ((1ULL << config::TLB_SET_BITS) - 1) << PGBITS;
        static constexpr word_t TLB_TAG_MASK = ((1ULL << TLB_TAG_BITS) - 1) << (PGBITS + config::TLB_SET_BITS);

    public:
        Addr() : addr(0) {}
        Addr(word_t addr) : addr(addr) {}
        Addr(const Addr &addr) : addr(addr.addr) {}

        word_t get() const { return addr; }

        bool operator==(const Addr &other) const { return addr == other.addr; }
        bool operator!=(const Addr &other) const { return addr != other.addr; }
        bool operator<(const Addr &other) const { return addr < other.addr; }
        bool operator>(const Addr &other) const { return addr > other.addr; }
        bool operator<=(const Addr &other) const { return addr <= other.addr; }
        bool operator>=(const Addr &other) const { return addr >= other.addr; }

        operator word_t() const { return addr; }

        word_t icache_set() const {
            return (addr & ICACHE_SET_MASK) >> 1;
        }

        word_t icache_tag() const {
            return addr & ICACHE_TAG_MASK;
        }

        word_t tlb_set() const {
            return (addr & TLB_SET_MASK) >> PGBITS;
        }

        word_t tlb_tag() const {
            return addr & TLB_TAG_MASK;
        }

        word_t tlb_off() const {
            return addr & TLB_OFF_MASK;
        }

        word_t tlb_tag_and_set() const {
            return addr & ~TLB_OFF_MASK;
        }

        word_t vpn(unsigned int level, unsigned int vpnbits) const {
            return (addr >> (PGBITS + level * vpnbits)) & ((1 << vpnbits) - 1);
        }

    };

    using addr_t = Addr;
}

#endif
