#ifndef __KDB_H__
#define __KDB_H__

#include "device/uart.h"
#include "isa/word.h"
#include "cpu/cpu.h"
#include "memory/memory.h"

#include <ostream>
#include <string>
#include <map>
#include <unordered_set>
#include <vector>

namespace kdb {
    void init();
    void deinit();

    // kdb command line
    void cmd_init();
    int run_cmd_mainloop();
    int run_command(const std::string &cmd);
    int run_source_file(const std::string &filename);

    // CPU execution
    extern CPU *cpu;
    extern int returnCode; // set when a core halt
    void reset_cpu();
    int run_cpu();
    int step_core(Core *core);

    // Memory
    extern Memory *memory;
    void init_memory();
    void deinit_memory();

    // Uart
    namespace uart{
        extern std::vector<Uart16650 *> list;
        bool add(word_t base, std::ostream &os);
        bool add(word_t base, const std::string &ip, int port);
    };

    // Breakpoint
    extern std::unordered_set<word_t> breakpointSet;
    extern bool brkTriggered;
    void add_breakpoint(word_t addr);

    // ELF format
    // Load ELF to memory, return 0 if failed and entry else.
    word_t load_elf(const std::string &filename);
    extern std::map<word_t, std::string> symbolTable;
    extern word_t programEntry;

    word_t string_to_addr(const std::string &s, bool &success);
    
} // kdb

#endif
