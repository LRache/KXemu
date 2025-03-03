#include "cpu/core.h"
#include "kdb/kdb.h"
#include "macro.h"
#include "word.h"

#include <iostream>

using namespace kxemu;
using kxemu::kdb::word_t;

std::unordered_set<word_t> kdb::breakpointSet;
bool kdb::brkTriggered = false;

void kdb::reset_cpu() {
    cpu->reset(kdb::programEntry);
    kdb::returnCode = 0;
}

static int print_halt(unsigned int coreID) {
    auto core = kdb::cpu->get_core(coreID);
    int r;
    if (core->is_error()) {
        std::cout << FMT_FG_RED_BLOD "Error" FMT_FG_BLUE_BLOD " at pc=" << FMT_STREAM_WORD(core->get_halt_pc()) << FMT_FG_RESET << std::endl;
        r = 1;
        kdb::returnCode = 1;
    } else if (core->is_halt()) {
        r = core->get_halt_code();
        if (r == 0) {
            std::cout << FMT_FG_BLUE_BLOD << "Core" << coreID << " " FMT_FG_GREEN_BLOD "HIT GOOD TRAP" FMT_FG_BLUE_BLOD " at pc=" << FMT_STREAM_WORD(core->get_halt_pc()) << FMT_FG_RESET << std::endl;
        } else {
            std::cout << FMT_FG_BLUE_BLOD << "Core" << coreID << " " FMT_FG_RED_BLOD "HIT BAD TRAP" FMT_FG_BLUE_BLOD " at pc=" << FMT_STREAM_WORD(core->get_halt_pc()) << FMT_FG_RESET << std::endl;
        }
        kdb::returnCode = r;
    } else {
        r = 0;
    }
    return r;
};

int kdb::step_core(unsigned int coreID) {
    auto core = cpu->get_core(coreID);
    word_t pc = core->get_pc();
    core->step();
    if (core->is_halt()) {
        print_halt(coreID);
    }
    if (breakpointSet.find(pc) != breakpointSet.end()) {
        brkTriggered = true;
    }
    return 0;
}

int kdb::run_cpu() {
    unsigned int n = breakpointSet.size();
    word_t *breakpoints = new word_t[n];
    int i = 0;
    for (auto it = breakpointSet.begin(); it != breakpointSet.end(); it++) {
        breakpoints[i++] = *it;
    }
    cpu->run(true, breakpoints, n);
    delete[] breakpoints;

    for (unsigned int i = 0; i < cpu->core_count(); i++) {
        print_halt(i);
    }

    return 0;
}

void kdb::add_breakpoint(word_t addr) {
    breakpointSet.insert(addr);
}

bool kdb::remove_breakpoint(word_t addr) {
    return breakpointSet.erase(addr) > 0;
}
