#include "kdb/kdb.h"
#include "kdb/cmd.h"
#include "isa/isa.h"
#include "utils/disasm.h"

#include <iostream>

int cmd::reset(const args_t &) {
    kdb::cpu->reset();
    return 0;
}

int cmd::step(const args_t &args) {
    std::string ns = args[1]; // step count
    unsigned long n;
    try {
        n = std::stoul(ns);
    } catch (std::invalid_argument &) {
        std::cout << "Invalid step count: " << ns << std::endl;
        return cmd::InvalidArgs;
    } catch (std::out_of_range &) {
        std::cout << "Step count out of range: " << ns << std::endl;
        return cmd::InvalidArgs;
    }

    Core *core = cmd::currentCore;
    for (unsigned long i = 0; i < n; i++) {
        if (!core->is_running()) {
            break;
        }

        // disassemble
        word_t pc = core->get_pc();
        uint8_t *mem = kdb::memory->get_ptr(pc);
        if (mem != nullptr) {
            unsigned int instLength;
            std::string inst = disasm::disassemble(mem, MAX_INST_LEN, pc, instLength);
            std::cout << FMT_STREAM_WORD(pc) << ":";
            for (unsigned int j = 0; j < instLength; j++) {
                std::cout << " " << std::hex << std::setw(2) << std::setfill('0') << (int)mem[j];
            }
            std::cout << inst << std::endl;
        } else {
            std::cout << "Unsupport to disassemble at pc =" << FMT_STREAM_WORD(pc) << std::endl;
        }

        // core step
        kdb::step_core(core);
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
