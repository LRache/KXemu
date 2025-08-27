/***************************************************************
 * Project Name: KXemu
 * File Name: include/cpu/cpu.hpp
 * Description: Define interface of the cpu and the cpu core for
                different ISA.
 ***************************************************************/

#ifndef __KXEMU_CPU_CPU_HPP__
#define __KXEMU_CPU_CPU_HPP__

#include "cpu/core.hpp"
#include "device/bus.hpp"

namespace kxemu::cpu {

template <typename word_t>
class CPU {
public:
    // flags for extension features
    virtual void init(device::Bus *memory, int flags, unsigned int coreCount) = 0;
    virtual void reset(word_t pc) = 0;
    virtual void step() = 0;
    virtual void run(bool blocked=false, const word_t *breakpoints=nullptr, unsigned int n=0) = 0;
    virtual void join() = 0;
    virtual bool is_running() = 0;

    virtual void set_debug_mode(bool debug) {}

    virtual unsigned int core_count() = 0;
    virtual Core<word_t> *get_core(unsigned int coreID) = 0;

    virtual ~CPU() = default;
};

} // namespace kxemu::cpu

#endif
