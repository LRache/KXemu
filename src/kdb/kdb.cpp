#include "kdb/kdb.h"
#include "cpu/cpu.h"
#include "isa/isa.h"
#include "log.h"
#include "isa/isa.h"
#include "utils/utils.h"

#include <fstream>
#include <string>
#include <iostream>

using namespace kxemu;
using kxemu::cpu::CPU;
using kxemu::kdb::word_t;

CPU<word_t> *kdb::cpu = nullptr;
word_t kdb::programEntry;
int kdb::returnCode = 0;

void kdb::init(unsigned int coreCount) {
    init_bus();

    logFlag = DEBUG | INFO | WARN | PANIC;

    cpu = isa::new_cpu();
    cpu->init(bus, -1, coreCount);
    cpu->reset(0x80000000);
    kdb::programEntry = 0x80000000;

    INFO("Init %s CPU", isa::get_isa_name());
}

void kdb::deinit() {
    delete cpu;
    cpu = nullptr;
    deinit_bus();
    uart::deinit();
}

// run .kdb source file to exec kdb command
int kdb::run_source_file(const std::string &filename) {
    std::ifstream f;
    f.open(filename, std::ios::in);
    if (!f.is_open()) {
        std::cerr << "FileNotFound: No such file: " << filename << std::endl;
        return 1;
    }

    std::string cmdLine;
    while (std::getline(f, cmdLine)) {
        run_command(cmdLine);
    }
    return 0;
}

word_t kdb::string_to_addr(const std::string &s, bool &success) {
    success = false;
    word_t addr;
    
    addr = utils::string_to_unsigned(s, success);
    if (success) {
        return addr;
    }
    
    for (auto iter : kdb::symbolTable) {
        if (iter.second == s) {
            addr = iter.first;
            success = true;
        }
    }

    return addr;
}
