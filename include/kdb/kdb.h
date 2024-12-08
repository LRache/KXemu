#ifndef __KDB_H__
#define __KDB_H__

#include "cpu/cpu.h"
#include "memory/memory.h"

namespace kdb {
    void init();
    void init_memory();
    void deinit();
    void deinit_memory();

    void run_kdb();
    int run_cpu();

    extern Memory *memory;
    extern CPU *cpu;
}

#endif
