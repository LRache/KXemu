#ifndef __KXEMU_CPU_RISCV_PLATFORM_HPP__
#define __KXEMU_CPU_RISCV_PLATFORM_HPP__

#include "cpu/riscv/aclint.hpp"
#include "cpu/riscv/cpu.hpp"
#include "cpu/riscv/host-timer.hpp"
#include "cpu/riscv/plic.hpp"
#include "device/bus.hpp"

namespace kxemu::cpu {

// The built-in virt-like platform: a host-clock timer, the ACLINT and the PLIC.
// Attach it after cpu->init(). The CPU uses the timer until it is destroyed, so
// destroy the CPU first; the bus keeps the device mappings, so destroy the bus
// with or after the platform.
class RVPlatform {
public:
    HostPlatformTimer timer;
    device::AClint aclint;
    device::PLIC plic;

    ~RVPlatform();

    // Attach the timer to cpu, map ACLINT and PLIC on bus and make the PLIC the
    // bus interrupt controller.
    void attach(RVCPU *cpu, device::Bus *bus);
    // Reset device state and rewind mtime, alongside cpu->reset().
    void reset();

private:
    device::Bus *bus = nullptr;
};

} // namespace kxemu::cpu

#endif
