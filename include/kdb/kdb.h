#ifndef __KXEMU_KDB_KDB_H__
#define __KXEMU_KDB_KDB_H__

#include "device/mmio.h"
#include "device/uart.h"
#include "cpu/cpu.h"
#include "device/bus.h"
#include "isa/isa.h"

#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <map>
#include <unordered_set>
#include <vector>

namespace kxemu::kdb {
    using word_t = isa::word_t;
    static constexpr unsigned int WORD_BITS = sizeof(word_t) * 8;
    static constexpr unsigned int WORD_WIDTH = WORD_BITS / 4;
    
    void init(unsigned int coreCount);
    void deinit();

    // GDB connection
    bool run_gdb(const std::string &addr);

    // CPU execution
    extern cpu::CPU<word_t> *cpu;
    extern int returnCode; // set when a core halt
    void reset_cpu();
    void reset_cpu(word_t entry);
    int run_cpu();
    int step_core(unsigned int coreID);

    // Device
    extern device::Bus *bus;
    namespace device {
        void init();
        void add(kxemu::device::MMIODev *dev);
        void deinit();
    }

    // Uart
    namespace uart{
        extern std::vector<kxemu::device::Uart16650 *> list;
        bool add(unsigned int id, word_t base, std::ostream &os);
        bool add(unsigned int id, word_t base, const std::string &ip, int port);
        bool puts(std::size_t index, const std::string &s);

        void deinit();
    };

    // Breakpoint
    extern std::unordered_set<word_t> breakpointSet;
    extern bool brkTriggered;
    void add_breakpoint(word_t addr);
    bool remove_breakpoint(word_t addr);

    // ELF format
    std::optional<word_t> load_elf(const std::string &filename);
    std::optional<std::string> addr_match_symbol(word_t addr, word_t &offset);
    extern std::map<word_t, std::string> symbolTable;
    extern word_t programEntry;

    word_t string_to_addr(const std::string &s, bool &success);
} // kdb

#endif
