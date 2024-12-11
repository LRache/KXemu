#include "kdb/kdb.h"
#include "cpu/cpu.h"
#include "log.h"
#include "isa/isa.h"
#include "common.h"
#include "macro.h"

#include <fstream>
#include <string>
#include <iostream>

CPU *kdb::cpu = nullptr;
word_t kdb::programEntry;
int kdb::returnCode = 0;

void kdb::init() {
    init_memory();

    logFlag = DEBUG | INFO | WARN | PANIC;

    cpu = new ISA_CPU();
    cpu->init(memory, 0, 1);
    cpu->reset();
    programEntry = INIT_PC;

    INFO("Init %s CPU", ISA_NAME);
}

void kdb::deinit() {
    delete cpu;
    cpu = nullptr;
}

int kdb::run_cpu() {
    while (!cpu->has_break()) {
        cpu->step();
    }

    Core *core = cpu->get_core(0);
    if (core->is_error()) {
        INFO(FMT_FG_RED "Error" FMT_FG_BLUE "at pc=" FMT_WORD, core->get_trap_pc());
        return 1;
    }

    int r = core->get_trap_code();
    if (r == 0) {
        INFO(FMT_FG_GREEN "HIT GOOD TRAP " FMT_FG_BLUE "at pc=" FMT_WORD, core->get_trap_pc()); 
    } else {
        INFO(FMT_FG_RED "HIT BAD TRAP " FMT_FG_BLUE "at pc=" FMT_WORD, core->get_trap_pc());
    }
    return r;
}

int kdb::step_core(Core *core) {
    core->step();
    if (core->is_break()) {
        int r = core->get_trap_code();
        if (r == 0) {
            INFO(FMT_FG_GREEN "HIT GOOD TRAP " FMT_FG_BLUE "at pc=" FMT_WORD, core->get_trap_pc()); 
        } else {
            INFO(FMT_FG_RED "HIT BAD TRAP " FMT_FG_BLUE "at pc=" FMT_WORD, core->get_trap_pc());
        }
        kdb::returnCode = r;
    } else if (core->is_error()) {
        kdb::returnCode = core->get_trap_code();
    }
    return 0;
}

// run .kdb source file to exec kdb command
int kdb::run_source_file(const std::string &filename) {
    std::ifstream f;
    f.open(filename, std::ios::in);
    if (!f.is_open()) {
        std::cout << "FileNotFound: No such file: " << filename << std::endl;
        return 1;
    }

    std::string cmdLine;
    while (std::getline(f, cmdLine)) {
        run_command(cmdLine);
    }
    return 0;
}
