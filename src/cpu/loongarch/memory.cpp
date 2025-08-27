#include "cpu/loongarch/core.h"
#include "cpu/word.h"

using namespace kxemu::cpu;

bool LACore::memory_fetch() {
    word_t addr = this->pc;
    
    bool valid;
    uint32_t inst = this->bus->read(addr, 4, valid);
    if (valid) {
        this->inst = inst;
    }
    return valid;
}

bool LACore::memory_store(word_t addr, word_t data, int len) {
    return this->bus->write(addr, data, len);
}

word_t LACore::memory_load(word_t addr, int len, bool &success) {
    return this->bus->read(addr, len, success);
}
