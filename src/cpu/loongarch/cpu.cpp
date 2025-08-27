#include "cpu/loongarch/cpu.h"
#include "log.h"

using namespace kxemu::cpu;

LACPU::LACPU() {
    this->cores = nullptr;
    this->coreCount = 0;
}

LACPU::~LACPU() {
    delete[] this->cores;
}

void LACPU::init(device::Bus *bus, int flags, unsigned int coreCount) {
    this->cores = new LACore[coreCount];
    this->coreCount = coreCount;
    for (unsigned int i = 0; i < coreCount; i++) {
        this->cores[i].init(i, bus, flags);
    }
}

void LACPU::reset(word_t pc) {
    for (unsigned int i = 0; i < this->coreCount; i++) {
        this->cores[i].reset(pc);
    }
}

void LACPU::step() {
    for (unsigned int i = 0; i < this->coreCount; i++) {
        this->cores[i].step();
    }
}

void LACPU::run(bool blocked, const word_t *breakpoints, unsigned int n) {
    this->cores[0].run(breakpoints, n);
}

void LACPU::join() {}

bool LACPU::is_running() {
    for (unsigned int i = 0; i < this->coreCount; i++) {
        if (this->cores[i].is_running()) {
            return true;
        }
    }
    return false;
}

unsigned int LACPU::core_count() {
    return this->coreCount;
}

LACore *LACPU::get_core(unsigned int coreID) {
    if (coreID >= coreCount) {
        PANIC("Core ID %d is out of range", coreID);
    }
    return &cores[coreID];
}
