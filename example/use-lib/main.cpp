#include "kxemu/cpu/riscv/cpu.h"
#include "kxemu/device/bus.h"
#include "kxemu/device/memory.h"
#include <iostream>
#include <cstdint>

using namespace kxemu;

int main() {
    device::Bus bus;
    device::Memory memory(0x1000);
    bus.add_memory_map("mem", 0x80000000, 0x1000, &memory);
    cpu::RVCPU cpu;
    cpu.init(&bus, 0, 1);
    cpu.reset(0x80000000);
    auto core = cpu.get_core(0);
    
    *(uint32_t *)memory.get_ptr(0) = 0x00100093; // addi x1, x0, 1
    core->step();
    std::cout << "x1=" << core->get_gpr(1) << std::endl;

    return 0;
}