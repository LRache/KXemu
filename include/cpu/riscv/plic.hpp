#ifndef __KXEMU_CPU_RISCV_PLIC_HPP__
#define __KXEMU_CPU_RISCV_PLIC_HPP__

#include "device/intc.hpp"
#include "device/mmio.hpp"
#include "cpu/riscv/def.hpp"

#include <cstdint>
#include <mutex>
#include <span>

namespace kxemu::device {

// Level-triggered PLIC with 32 sources (source 0 is reserved) and 32 contexts.
// Devices push levels through set_irq(); each context output drives MEIP or
// SEIP of its hart whenever it changes, waking the hart if it sleeps in WFI.
class PLIC : public device::MMIODev, public device::InterruptController {
private:
    static constexpr unsigned int SOURCE_COUNT  = 32;
    static constexpr unsigned int CONTEXT_COUNT = 32;

    // Guards all state below. Core IRQ methods are called with it held so that
    // concurrent updates reach the cores in order; cores never call back here.
    std::mutex mtx;

    uint32_t priority[SOURCE_COUNT];
    uint32_t level   = 0; // Current device levels
    uint32_t pending = 0; // Gateway pending bits
    uint32_t claimed = 0; // Claimed and not yet completed

    struct TargetContext {
        uint32_t enable;
        uint32_t threshold;
        bool output;
        cpu::RVCore *core;
    };
    TargetContext targetContexts[CONTEXT_COUNT];
    unsigned int activeContexts = 0; // Contexts 0..2*harts-1 have a hart attached

    // Requires mtx. Returns the best claimable source of a context, or 0.
    unsigned int best_source(const TargetContext &target) const;
    // Requires mtx. Recompute every context output and notify changed cores.
    void update_outputs();
    void set_output(unsigned int contextID, bool output);

public:
    PLIC();

    // Hart i drives contexts 2i (M-mode) and 2i+1 (S-mode).
    void init(std::span<cpu::RVCore *const> cores);
    // Clear registers and claims; source levels are kept.
    void reset() override;
    word_t read(word_t offset, word_t size, bool &valid) override;
    bool write(word_t offset, word_t data, word_t size) override;

    void set_irq(unsigned int irq, bool level) override;

    const char *get_type_name() const override {
        return "PLIC";
    }
};

} // namespace kxemu::device

#endif
