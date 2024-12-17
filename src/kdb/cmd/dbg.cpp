#include "isa/word.h"
#include "kdb/kdb.h"
#include "kdb/cmd.h"
#include "utils/disasm.h"
#include "utils/utils.h"

#include <exception>
#include <iostream>

int cmd::reset(const args_t &) {
    kdb::reset_cpu();
    return 0;
}

static void output_disassemble(word_t pc) {
    uint8_t *mem = kdb::memory->get_ptr(pc);
    auto symbol = kdb::symbolTable.find(pc);
    if (symbol != kdb::symbolTable.end()) {
        std::cout << symbol->second << ":" << std::endl;
    }
    if (mem == nullptr) {
        std::cout << "Unsupport to disassemble at pc=" << FMT_STREAM_WORD(pc) << std::endl;
        return;
    } else {
        unsigned int instLength;
        std::string inst = disasm::disassemble(mem, MAX_INST_LEN, pc, instLength);
        std::cout << FMT_STREAM_WORD(pc) << ":";
        for (unsigned int j = 0; j < instLength; j++) {
            std::cout << " " << std::hex << std::setw(2) << std::setfill('0') << (int)mem[j];
        }
        std::cout << inst << std::endl;
    }
}

int cmd::step(const args_t &args) {
    unsigned long n; // step count
    if (args.size() == 1) {
        n = 1;
    } else {
        std::string ns = args[1];
        try {
            n = std::stoul(ns);
        } catch (std::invalid_argument &) {
            std::cout << "Invalid step count: " << ns << std::endl;
            return cmd::InvalidArgs;
        } catch (std::out_of_range &) {
            std::cout << "Step count out of range: " << ns << std::endl;
            return cmd::InvalidArgs;
        }
    }

    Core *core = cmd::currentCore;
    kdb::brkTriggered = false;
    for (unsigned long i = 0; i < n; i++) {
        if (!core->is_running()) {
            break;
        }

        word_t pc = core->get_pc();

        // core step
        kdb::step_core(core);

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
    if (!kdb::cpu->is_running()) {
        std::cout << "CPU is not running." << std::endl;
    } else {
        kdb::run_cpu();
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
    word_t addr;
    try {
        addr = utils::string_to_word(addrStr);
    } catch (const std::exception &) {
        std::cout << "Invalid argument: " << addrStr << std::endl;
        return cmd::InvalidArgs;
    }
    
    kdb::add_breakpoint(addr);
    std::cout << "Set breakpoint at " << FMT_STREAM_WORD(addr) << std::endl;
    
    return cmd::Success;
}
