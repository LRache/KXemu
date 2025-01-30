#include "cpu/core.h"
#include "kdb/kdb.h"
#include "macro.h"
#include "log.h"

#include <iostream>

using namespace kxemu;
using kxemu::kdb::word_t;
using kxemu::cpu::Core;

std::unordered_set<word_t> kdb::breakpointSet;
bool kdb::brkTriggered = false;

void kdb::reset_cpu() {
    cpu->reset(kdb::programEntry);
    kdb::returnCode = 0;
}

int static output_and_set_trap(Core<word_t> *core) {
    int r;
    if (core->is_error()) {
        std::cout << FMT_FG_RED_BLOD "Error" FMT_FG_BLUE_BLOD " at pc=" << FMT_STREAM_WORD(core->get_halt_pc()) << FMT_FG_RESET << std::endl;
        r = 1;
        kdb::returnCode = 1;
    } else if (core->is_halt()) {
        r = core->get_halt_code();
        if (r == 0) {
            std::cout << FMT_FG_GREEN_BLOD "HIT GOOD TRAP" FMT_FG_BLUE_BLOD " at pc=" << FMT_STREAM_WORD(core->get_halt_pc()) << FMT_FG_RESET << std::endl;
        } else {
            std::cout << FMT_FG_RED_BLOD "HIT BAD TRAP" FMT_FG_BLUE_BLOD " at pc=" << FMT_STREAM_WORD(core->get_halt_pc()) << FMT_FG_RESET << std::endl;
        }
        kdb::returnCode = r;
    } else {
        r = 0;
    }
    return r;
};

int kdb::step_core(Core<word_t> *core) {
    word_t pc = core->get_pc();
    core->step();
    output_and_set_trap(core);
    if (breakpointSet.find(pc) != breakpointSet.end()) {
        brkTriggered = true;
    }
    return 0;
}

// NOTE: This function only support single core CPU
int kdb::run_cpu() {
    // while (cpu->is_running()) {
    //     word_t pc = cpu->get_core(0)->get_pc();
    //     if (breakpointSet.find(pc) != breakpointSet.end()) {
    //         brkTriggered = true;
    //         break;
    //     }
    //     cpu->step();
    // }

    auto core = cpu->get_core(0);

    unsigned int n = breakpointSet.size();
    word_t *breakpoints = new word_t[n];
    int i = 0;
    for (auto it = breakpointSet.begin(); it != breakpointSet.end(); it++) {
        breakpoints[i++] = *it;
    }
    core->run(breakpoints, n);
    delete[] breakpoints;

    brkTriggered = core->is_break();

    return output_and_set_trap(core);
}

void kdb::add_breakpoint(word_t addr) {
    breakpointSet.insert(addr);
}
