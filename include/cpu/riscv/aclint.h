#ifndef __KXEMU_CPU_RISCV_CLINT_H__
#define __KXEMU_CPU_RISCV_CLINT_H__

#include "device/mmio.h"
#include "utils/task-timer.h"

#include <cstdint>
#include <mutex>

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
    struct CoreObject {
        cpu::RVCore *core;         // Pointer to the core object

        bool msip;
        bool ssip;
        bool mtip;
        bool stip;

        uint64_t mtimecmp;
        unsigned int mtimerID;
        unsigned int stimerID;

        unsigned int pendingInterrupts;
    };
    CoreObject *coreObjects;
    std::mutex mtx;

    AClint();
    ~AClint();

    void init(cpu::RVCore *cores, unsigned int coreCount);
    void reset() override;

    word_t read(word_t addr, word_t size, bool &success) override;
    bool write(word_t addr, word_t value, word_t size) override;
    void update() override;

    void start_timer();
    void stop_timer();
    uint64_t get_uptime();
    void register_stimer(unsigned int coreID, uint64_t stimecmp);

    const char *get_type_name() const override;
private:
    unsigned int coreCount;

    bool timerRunning;
    uint64_t bootTime;

    utils::TaskTimer taskTimer;
    void update_core_mtimecmp(unsigned int coreID);
};

} // namespace kxemu::cpu

#endif
