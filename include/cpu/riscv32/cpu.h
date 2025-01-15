#ifndef __KXEMU_CPU_RISCV32_CPU_H__
#define __KXEMU_CPU_RISCV32_CPU_H__

#include "cpu/cpu.h"
#include "cpu/riscv32/core.h"

namespace kxemu::cpu {

class RV32CPU : public CPU {
private:    
    RV32Core *core;
    int coreCount;

public:
    void init(device::Bus *bus, int flags, int coreCount) override;
    void reset(word_t pc) override;
    void step() override;
    bool is_running() override;

    int core_count() override;
    Core *get_core(int coreID) override;

    ~RV32CPU() override;
};

} // namespace kxemu::cpu

#endif
