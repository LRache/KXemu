#include "kxemu/cpu/riscv/cpu.h"
#include "kxemu/device/bus.h"
#include <iostream>
#include <cstdint>

using namespace kxemu;

int main() {
    device::Bus bus;
    bus.add_memory_map(0x80000000, 0x1000);
    cpu::RVCPU cpu;
    cpu.init(&bus, 0, 1);
    cpu.reset(0x80000000);
    auto core = cpu.get_core(0);
    
    *(uint32_t *)bus.get_ptr(0x80000000) = 0x00100093; // addi x1, x0, 1
    core->step();
    std::cout << "x1=" << core->get_gpr(1) << std::endl;

    return 0;
}