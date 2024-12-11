#ifndef __KDB_H__
#define __KDB_H__

#include "common.h"
#include "cpu/cpu.h"
#include "memory/memory.h"

#include <string>

namespace kdb {
    void init();
    void deinit();

    void init_memory();
    void deinit_memory();

    void cmd_init();
    void run_cmd_mainloop();
    int run_command(const std::string &cmd);
    int run_cpu();

    // Load elf to memory, return 0 if failed and entry else.
    word_t load_elf(const std::string &filename);

    extern Memory *memory;
    extern CPU *cpu;
}

#endif
