#ifndef __KXEMU_CPU_RISCV_CLINT_HPP__
#define __KXEMU_CPU_RISCV_CLINT_HPP__

#include "cpu/riscv/platform-timer.hpp"
#include "device/mmio.hpp"

#include <mutex>
#include <span>

namespace kxemu::cpu {
    class RVCore;
}

namespace kxemu::device {

// This is an implementation of the Advanced Core Local Interruptor (ACLINT) for RISC-V.
// See https://github.com/riscv/riscv-aclint/releases/download/v1.0-rc4/riscv-aclint-1.0-rc4.pdf
// offset          | Device   | Description
// 0x0000 - 0x3fff | MSWI     | Machine-mode Software Interrupt Device
// 0x4000 - 0x7ff0 | MTIMECMP |
// 0xbff8 - 0xbfff | MTIME    |
// 0xc000 - 0xffff | SSWI     | Supervisor-mode Software Interrupt Device
class AClint : public device::MMIODev {
public:
    AClint();
    ~AClint();

    void init(std::span<cpu::RVCore *const> cores, cpu::PlatformTimer *timer);
    void reset() override;

    word_t read(word_t addr, word_t size, bool &success) override;
    bool write(word_t addr, word_t value, word_t size) override;

    const char *get_type_name() const override;
private:
    struct CoreObject {
        cpu::RVCore *core; // Pointer to the core object

        bool msip;
        bool ssip;

        cpu::TimerComparator mtimer; // MTIMECMP, drives MTIP
    };
    CoreObject *coreObjects;
    unsigned int coreCount;
    cpu::PlatformTimer *timer;

    // Serializes MMIO accesses, including the split MTIMECMP writes on RV32.
    std::mutex mtx;
};

} // namespace kxemu::device

#endif
