#include "kdb/kdb.hpp"
#include "cpu/cpu.hpp"
#include "isa/isa.hpp"
#include "utils/utils.hpp"
#include "log.h"

#include <string>

using namespace kxemu;
using kxemu::cpu::CPU;
using kxemu::kdb::word_t;

CPU<word_t> *kdb::cpu = nullptr;
word_t kdb::programEntry;
int kdb::returnCode = 0;

void kdb::init(unsigned int coreCount) {
    device::init();

    logFlag = DEBUG | INFO | WARN | PANIC;

    isa::init();
    cpu = isa::new_cpu();
    cpu->init(bus, -1, coreCount);
    cpu->reset(0x80000000);
    kdb::programEntry = 0x80000000;

    INFO("Init %s CPU", isa::get_isa_name());
}

void kdb::deinit() {
    delete cpu;
    cpu = nullptr;
    device::deinit();
    uart::deinit();
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
