#ifndef __KDB_H__
#define __KDB_H__

#include "cpu/cpu.h"
#include "isa/isa.h"
#include "memory/memory.h"


namespace kdb {
    void init(const ISA &isa);
    void init_memory();
    void deinit();
    void deinit_memory();

    void run_kdb();
    int run_cpu();

    extern Memory *memory;
    extern CPU *cpu;

    namespace config {
        extern bool is64ISA;
    }
}

#endif
