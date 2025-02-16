#include "cpu/loongarch/core.h"
#include "cpu/word.h"
#include "device/bus.h"
#include <cstring>

using namespace kxemu::cpu;

LACore::LACore() {

}

void LACore::init(unsigned int coreID, device::Bus *bus, int flags) {
    this->state = IDLE;
    this->bus = bus;
}

void LACore::reset(word_t entry) {
    this->state = IDLE;
    this->pc = entry;
    std::memset(this->gpr, 0x1a, sizeof(this->gpr));
    this->gpr[0] = 0;
}

void LACore::set_gpr(unsigned int index, word_t value) {
    this->gpr[index] = value;
    this->gpr[0] = 0;
}

bool LACore::is_error() {
    return this->state == ERROR;
}

bool LACore::is_break() {
    return this->state == BREAKPOINT;
}

bool LACore::is_running() {
    return this->state == RUNNING;
}

bool LACore::is_halt() {
    return this->state == HALT;
}

word_t LACore::get_pc() {
    return this->pc;
}

word_t LACore::get_gpr(int idx) {
    return this->gpr[idx];
}

word_t LACore::get_register(const std::string &name, bool &success) {
    success = false;
    return 0;
}

word_t LACore::get_halt_pc() {
    return this->haltPC;
}

word_t LACore::get_halt_code() {
    return this->haltCode;
}
