/***************************************************************
 * Project Name: KXemu
 * File Name: include/cpu/cpu.h
 * Description: Define interface of the cpu and the cpu core for
                different ISA.
 ***************************************************************/

#ifndef __CPU_H__
#define __CPU_H__

#include "memory/memory.h"
#include "common.h"

class Core {
public:
    virtual bool is_error() = 0;
    virtual bool is_break() = 0;
    virtual bool is_running() = 0;
    virtual ~Core() = default;

    virtual void step() = 0;
    
    virtual word_t get_pc() = 0;
    virtual word_t get_gpr(int idx) = 0;

    virtual word_t get_trap_code() = 0;
    virtual word_t get_trap_pc()   = 0;
    virtual word_t get_halt_pc()   = 0;
};

class CPU {
public:
    // flags for extension features
    virtual void init(Memory *memory, int flags, int coreCount) = 0;
    virtual void reset(word_t pc) = 0;
    virtual void step() = 0;
    virtual bool is_running() = 0;

    virtual int core_count() = 0;
    virtual Core *get_core(int coreID) = 0;

    virtual ~CPU() = default;
};

#endif
