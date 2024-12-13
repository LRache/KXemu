#include "kdb/kdb.h"
#include "cpu/cpu.h"
#include "log.h"
#include "isa/isa.h"
#include "isa/word.h"
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
    cpu->reset(INIT_PC);
    programEntry = INIT_PC;

    INFO("Init %s CPU", ISA_NAME);
}

void kdb::deinit() {
    delete cpu;
    cpu = nullptr;
}

int static output_and_set_trap(Core *core) {
    int r;
    if (core->is_error()) {
        std::cout << FMT_FG_RED "Error" FMT_FG_BLUE " at pc=" << FMT_STREAM_WORD(core->get_trap_pc()) << FMT_FG_RESET << std::endl;
        r = 1;
        kdb::returnCode = 1;
    } else if (core->is_break()) {
        r = core->get_trap_code();
        if (r == 0) {
            std::cout << FMT_FG_GREEN "HIT GOOD TRAP" FMT_FG_BLUE " at pc=" << FMT_STREAM_WORD(core->get_trap_pc()) << FMT_FG_RESET << std::endl;
        } else {
            std::cout << FMT_FG_RED "HIT BAD TRAP" FMT_FG_BLUE " at pc=" << FMT_STREAM_WORD(core->get_trap_pc()) << FMT_FG_RESET << std::endl;
        }
        kdb::returnCode = r;
    } else {
        r = 0;
    }
    return r;
};

int kdb::run_cpu() {
    while (cpu->is_running()) {
        cpu->step();
    }

    Core *core = cpu->get_core(0);
    return output_and_set_trap(core);
}

int kdb::step_core(Core *core) {
    core->step();
    output_and_set_trap(core);
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
