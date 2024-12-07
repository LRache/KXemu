#include "kdb/kdb.h"
#include "cpu/cpu.h"
#include "log.h"
#include "isa/isa.h"
#include "isa/riscv/cpu.h"
#include "common.h"
#include <cstdint>

CPU *kdb::cpu = nullptr;
bool kdb::config::is64ISA;

void kdb::init(const ISA &isa) {
    switch (isa) {
        case RISCV32:
            cpu = new RV32CPU();
            kdb::config::is64ISA = false;
            break;
        default:
            PANIC("Unsupported ISA %s", ISA_NAME[isa]);
    }

    init_memory();

    cpu->init(memory, 0, 1);
    cpu->reset();

    INFO("Init %s CPU", ISA_NAME[isa]);
}

void kdb::deinit() {
    delete cpu;
    cpu = nullptr;
}

int kdb::run_cpu() {
    while (!cpu->has_break()) {
        cpu->step();
    }

    Core *core = cpu->getCore(0);
    if (core->is_error()) {
        if (config::is64ISA) {
            INFO(FMT_COLOR_RED "Error" FMT_COLOR_BLUE "at pc=" FMT_WORD_64, core->trapPC);
        } else {
            INFO(FMT_COLOR_RED "Error" FMT_COLOR_BLUE "at pc=" FMT_WORD_32, (uint32_t)core->trapPC);
        }
        return 1;
    }

    int r = core->trapCode;
    if (r == 0) {
        if (config::is64ISA) {
            INFO(FMT_COLOR_GREEN "HIT GOOD TRAP " FMT_COLOR_BLUE "at pc=" FMT_WORD_64, core->trapPC);
        }
        else {
            INFO(FMT_COLOR_GREEN "HIT GOOD TRAP " FMT_COLOR_BLUE "at pc=" FMT_WORD_32, (uint32_t)core->trapPC); 
        }
    } else {
        if (config::is64ISA) {
            INFO(FMT_COLOR_RED "HIT BAD TRAP " FMT_COLOR_BLUE "at pc=" FMT_WORD_64, core->trapPC);
        } 
        else {
            INFO(FMT_COLOR_RED "HIT BAD TRAP " FMT_COLOR_BLUE "at pc=" FMT_WORD_32, (uint32_t)core->trapPC);
        }
    }
    return r;
}
