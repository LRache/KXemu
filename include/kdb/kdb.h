#ifndef __KDB_H__
#define __KDB_H__

#include "common.h"
#include "cpu/cpu.h"
#include "memory/memory.h"

#include <string>
#include <map>

namespace kdb {
    void init();
    void deinit();

    void init_memory();
    void deinit_memory();

    // kdb command line
    void cmd_init();
    int run_cmd_mainloop();
    
    int run_command(const std::string &cmd);
    int run_cpu();
    int step_core(Core *core);
    extern int returnCode; // set when a core halt

    int run_source_file(const std::string &filename);

    // ELF format
    // Load ELF to memory, return 0 if failed and entry else.
    word_t load_elf(const std::string &filename);
    extern std::map<word_t, std::string> symbolTable;
    extern word_t programEntry;

    extern Memory *memory;
    extern CPU *cpu;
} // kdb

#endif
