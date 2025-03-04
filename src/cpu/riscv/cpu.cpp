#include "cpu/riscv/cpu.h"
#include "cpu/riscv/def.h"
#include "device/bus.h"
#include "log.h"

#include <thread>

using namespace kxemu::cpu;
using namespace kxemu::device;

RVCPU::RVCPU() {
    this->cores = nullptr;
    this->coreCount = 0;
    this->coreThread = nullptr;
}

void RVCPU::init(Bus *bus, int flags, unsigned int coreCount) {
    this->cores = new RVCore[coreCount];
    for (unsigned int i = 0; i < coreCount; i++) {
        this->cores[i].init(i, bus, flags, &aclint, &plic);
    }
    this->aclint.init(this->cores, coreCount);
    this->plic.init(this->cores, coreCount);
    bus->add_mmio_map(ACLINT_BASE, ACLINT_SIZE, &aclint);
    bus->add_mmio_map(PLIC_BASE, PLIC_SIZE, &plic);
    this->coreCount = coreCount;
}

void RVCPU::reset(word_t pc) {
    for (unsigned int i = 0; i < coreCount; i++) {
        cores[i].reset(pc);
    }
    aclint.reset();
}

void RVCPU::step() {
    for (unsigned int i = 0; i < coreCount; i++) {
        cores[i].step();
    }
}

void RVCPU::core_thread_worker(unsigned int coreID, const word_t *breakpoints, unsigned int n) {
    cores[coreID].run(breakpoints, n);
}

void RVCPU::run(bool blocked, const word_t *breakpoints, unsigned int n) {
    aclint.start_timer();
    if (this->coreCount == 1) {
        cores[0].run(breakpoints, n);
        aclint.stop_timer();
    } else {
        this->coreThread = new std::thread[coreCount];
        for (unsigned int i = 0; i < coreCount; i++) {
            coreThread[i] = std::thread(&RVCPU::core_thread_worker, this, i, breakpoints, n);
        }

        if (blocked) {
            this->join();
        }
    }
}

void RVCPU::join() {
    if (coreThread == nullptr) {
        return ;
    }
    for (unsigned int i = 0; i < coreCount; i++) {
        coreThread[i].join();
    }
    delete []coreThread;
    aclint.stop_timer();
}

bool RVCPU::is_running() {
    return cores[0].is_running();
}

unsigned int RVCPU::core_count() {
    return coreCount;
}

RVCore *RVCPU::get_core(unsigned int coreID) {
    if (coreID >= coreCount) {
        PANIC("Core ID %d is out of range", coreID);
    }
    return &cores[coreID];
}

RVCPU::~RVCPU() {
    delete []cores;
}
