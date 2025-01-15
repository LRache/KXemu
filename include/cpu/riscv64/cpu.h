#ifndef __KXEMU_ISA_RISCV64_CPU_H__
#define __KXEMU_ISA_RISCV64_CPU_H__

#include "isa/riscv64/core.h"

class RV64CPU : public CPU {
private:
    RV64Core *core;
    int coreCount;

public:
    void init(Bus *memory, int flags, int coreCount) override;
    void reset(word_t pc) override;
    void step() override;
    bool is_running() override;

    int core_count() override;
    Core *get_core(int coreID) override;

    ~RV64CPU() override;
};

#endif
