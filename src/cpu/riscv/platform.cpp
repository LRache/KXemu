#include "cpu/riscv/platform.hpp"
#include "cpu/riscv/def.hpp"
#include "debug.h"

#include <vector>

using namespace kxemu::cpu;

RVPlatform::~RVPlatform() {
    // Callbacks reference the comparators, so stop before members are destroyed.
    this->timer.stop();
    if (this->bus != nullptr && this->bus->get_interrupt_controller() == &this->plic) {
        this->bus->set_interrupt_controller(nullptr);
    }
}

void RVPlatform::attach(RVCPU *cpu, device::Bus *bus) {
    Assert(this->bus == nullptr, "RVPlatform is already attached.");
    this->bus = bus;

    std::vector<RVCore *> cores;
    for (unsigned int i = 0; i < cpu->core_count(); i++) {
        cores.push_back(cpu->get_core(i));
    }

    cpu->set_platform_timer(&this->timer);
    this->aclint.init(cores, &this->timer);
    this->plic.init(cores);
    Assert(bus->add_mmio_map(ACLINT.BASE, ACLINT.SIZE, &this->aclint), "ACLINT map failed.");
    Assert(bus->add_mmio_map(PLIC.BASE, PLIC.SIZE, &this->plic), "PLIC map failed.");
    bus->set_interrupt_controller(&this->plic);
}

void RVPlatform::reset() {
    this->aclint.reset();
    this->plic.reset();
    this->timer.reset();
}
