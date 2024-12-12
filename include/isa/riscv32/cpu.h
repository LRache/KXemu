#ifndef __ISA_RISCV32_CPU_H__
#define __ISA_RISCV32_CPU_H__

#include "common.h"
#include "cpu/cpu.h"
#include "isa/riscv32/core.h"

class RV32CPU : public CPU {
private:    
    RV32Core *core;
    int coreCount;

public:
    void init(Memory *memory, int flags, int coreCount) override;
    void reset(word_t pc) override;
    void step() override;
    bool is_running() override;

    int core_count() override;
    Core *get_core(int coreID) override;

    ~RV32CPU() override;
};

#endif
