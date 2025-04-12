#include "cpu/cpu.h"
#include "word.h"
#include "kdb/kdb.h"
#include "kdb/cmd.h"
#include "utils/utils.h"
#include "macro.h"

#include <cstdint>
#include <iostream>
#include <optional>

using namespace kxemu;
using namespace kxemu::kdb;

int cmd::reset(const args_t &args) {
    if (args.size() != 1) {
        try {
            word_t entry = std::stoul(args[1], nullptr, 0);
            kdb::reset_cpu(entry);
        } catch (const std::exception &e) {
            std::cout << "Invalid entry point: " << args[1] << std::endl;
            return cmd::InvalidArgs;
        }
    } else {
        kdb::reset_cpu();
    }
    return 0;
}

static void output_disassemble(word_t pc) {
    bool valid;
    word_t paddr = kdb::cpu->get_core(0)->vaddr_translate(pc, valid);

    if (!valid) {
        std::cout << "Cannot access memory at pc= " << FMT_STREAM_WORD(pc) << "." << std::endl;
        return;
    }
    if (paddr != pc) {
        std::cout << "(vaddr=" << FMT_STREAM_WORD(pc) << ")";
    }
    pc = paddr;

    uint8_t *mem = (uint8_t *)kdb::bus->get_ptr(pc);
    uint64_t memSize = kdb::bus->get_ptr_length(pc);
    if (mem == nullptr) {
        std::cout << "Unsupport to disassemble at pc=" << FMT_STREAM_WORD(pc) << std::endl;
        return;
    } else {
        // find nearest symbol
        word_t symbolOffset = 0;
        auto symbolName = kdb::addr_match_symbol(pc, symbolOffset);
        
        uint64_t instLength;
        std::string inst = isa::disassemble(mem, memSize, pc, instLength);
        std::cout << FMT_STREAM_WORD(pc) << ": ";
        if (symbolName != std::nullopt) {
            std::cout << "<" << FMT_FG_YELLOW << symbolName.value() << FMT_FG_RESET << "+" << symbolOffset << "> ";
        }
        std::cout << "0x";
        for (int j = instLength - 1; j >= 0; j--) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)mem[j];
        }
        std::cout << inst << std::endl;
    }
}

int cmd::step(const args_t &args) {
    unsigned long n; // step count
    if (args.size() == 1) {
        n = 1;
    } else {
        bool s;
        n = utils::string_to_unsigned(args[1], s);
        if (!s) {
            std::cout << "Invalid step count: " << args[1] << std::endl;
            return cmd::InvalidArgs;
        }
    }

    auto core = kdb::cpu->get_core(cmd::currentCore);
    kdb::brkTriggered = false;
    for (unsigned long i = 0; i < n; i++) {
        if (core->is_halt()) {
            break;
        }

        word_t pc = core->get_pc();

        // core step
        kdb::step_core(cmd::currentCore);

        // disassemble
        output_disassemble(pc);

        if (kdb::brkTriggered) {
            std::cout << "Breakpoint at " << FMT_STREAM_WORD(pc) << " triggered."<< std::endl;
            break;
        }
    }
    return 0;
}

int cmd::run(const args_t &) {
    kdb::run_cpu();
    
    for (unsigned int i = 0; i < kdb::cpu->core_count(); i++) {
        auto core = kdb::cpu->get_core(i);
        if (core->is_break()) {
            std::cout << "Core " << i << ": Breakpoint at " << FMT_STREAM_WORD(core->get_pc()) << " triggered."<< std::endl;
        }
    }
    
    return 0;
}

int cmd::symbol(const cmd::args_t &) {
    if (kdb::symbolTable.empty()) {
        std::cout << "No symbol found" << std::endl;
        return cmd::Success;
    }
    
    std::cout << std::setfill(' ')
    << std::setw(16)  << "name" << " | "
    << std::setw(WORD_WIDTH + 2) << "addr"
    << std::endl;
    for (auto sym : kdb::symbolTable) {
        std::cout << std::setfill(' ')
        << std::setw(16) << sym.second << " | "
        << FMT_STREAM_WORD(sym.first) 
        << std::endl;
    }
    
    return cmd::Success;
}

int cmd::breakpoint(const cmd::args_t &args) {
    if (args.size() < 2) {
        std::cout << "Usage: breakpoint <addr>" << std::endl;
        return cmd::EmptyArgs;
    }
    
    std::string addrStr = args[1];
    bool success;
    word_t addr = string_to_addr(addrStr, success);
    if (!success) {
        std::cout << "Invalid argument: " << addrStr << std::endl;
        return cmd::InvalidArgs;
    }
    
    kdb::add_breakpoint(addr);
    std::cout << "Set breakpoint at " << FMT_STREAM_WORD(addr) << std::endl;
    
    return cmd::Success;
}
