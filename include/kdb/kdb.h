#ifndef __KDB_H__
#define __KDB_H__

#include "device/uart.h"
#include "isa/word.h"
#include "cpu/cpu.h"
#include "device/bus.h"

#include <cstddef>
#include <optional>
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

    // Bus
    extern Bus *bus;
    void init_bus();
    void deinit_bus();

    // Uart
    namespace uart{
        extern std::vector<Uart16650 *> list;
        bool add(word_t base, std::ostream &os);
        bool add(word_t base, const std::string &ip, int port);
        bool puts(std::size_t index, const std::string &s);
    };

    // Breakpoint
    extern std::unordered_set<word_t> breakpointSet;
    extern bool brkTriggered;
    void add_breakpoint(word_t addr);

    // ELF format
    // Load ELF to memory, return 0 if failed and entry else.
    word_t load_elf(const std::string &filename);
    std::optional<std::string> addr_match_symbol(word_t addr, word_t &offset);
    extern std::map<word_t, std::string> symbolTable;
    extern word_t programEntry;

    word_t string_to_addr(const std::string &s, bool &success);

    // GDB connection
    bool start_rsp(int port);
    
} // kdb

#endif
