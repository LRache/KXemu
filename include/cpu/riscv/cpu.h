#ifndef __KXEMU_CPU_RISCV_CPU_H__
#define __KXEMU_CPU_RISCV_CPU_H__

#include "cpu/cpu.h"
#include "cpu/riscv/aclint.h"
#include "cpu/riscv/core.h"
#include "utils/task-timer.h"

namespace kxemu::cpu {

#ifdef KXEMU_ISA64
    using word_t = uint64_t;
#else
    using word_t = uint32_t;
#endif

class RVCPU : public CPU<word_t> {
private:    
    RVCore *cores;
    unsigned int coreCount;
    
    AClint aclint;
    utils::TaskTimer taskTimer;
    
public:
    ~RVCPU() override;

    void init(device::Bus *bus, int flags, unsigned int coreCount) override;
    void reset(word_t pc) override;
    void step() override;
    bool is_running() override;

    unsigned int core_count() override;
    RVCore *get_core(unsigned int coreID) override;
};

} // namespace kxemu::cpu

#endif
