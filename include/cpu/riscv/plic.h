#ifndef __KXEMU_CPU_RISCV_PLIC_H__
#define __KXEMU_CPU_RISCV_PLIC_H__

#include "device/mmio.h"
#include "cpu/riscv/namespace.h"
#include "utils/spinlock.h"

#include <cstdint>

namespace kxemu::device {

class PLIC : public device::MMIODev {
private:
    Bus *bus;
    
    // We only support 32 targets context and 32 interrput sources.
    struct InterruptSource {
        uint32_t priority;
        bool pending;
        bool enable[32];
    };
    InterruptSource interruptSources[32];

    struct TargetContext {
        uint32_t threshold;
        uint32_t claim;
        cpu::RVCore *core;
    };
    TargetContext targetContexts[32];

    utils::SpinLock lock;

public:
    void init(cpu::RVCore *cores, unsigned int coreCount);
    void reset() override;
    word_t read(word_t offset, word_t size, bool &valid) override;
    bool write(word_t offset, word_t data, word_t size) override;
    void connect_to_bus(Bus *bus) override;

    void scan_and_set_interrupt(unsigned int hartid, int privMode);

    const char *get_type_name() const override {
        return "PLIC";
    }

    utils::SpinLock *get_lock() {
        return &this->lock;
    }
};

} // namespace kxemu::cpu

#endif
