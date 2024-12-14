#include "kdb/kdb.h"
#include "log.h"

#include <iostream>

std::unordered_set<word_t> kdb::breakpointSet;
bool kdb::brkTriggered = false;

void kdb::reset_cpu() {
    cpu->reset(kdb::programEntry);
    kdb::returnCode = 0;
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

int kdb::step_core(Core *core) {
    word_t pc = core->get_pc();
    core->step();
    output_and_set_trap(core);
    if (breakpointSet.find(pc) != breakpointSet.end()) {
        brkTriggered = true;
    }
    return 0;
}

int kdb::run_cpu() {
    while (cpu->is_running()) {
        cpu->step();
    }

    Core *core = cpu->get_core(0);
    return output_and_set_trap(core);
}

void kdb::add_breakpoint(word_t addr) {
    breakpointSet.insert(addr);
}
