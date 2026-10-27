#ifndef __KXEMU_CPU_RISCV_CPU_HPP__
#define __KXEMU_CPU_RISCV_CPU_HPP__

#include "cpu/cpu.hpp"
#include "cpu/riscv/platform-timer.hpp"
#include "cpu/riscv/core.hpp"

#include <thread>

namespace kxemu::cpu {

class RVCPU : public CPU<word_t> {
private:    
    RVCore *cores;
    unsigned int coreCount;
    std::thread *coreThread;
    void core_thread_worker(unsigned int coreID, const word_t *breakpoints, unsigned int n);
    
    PlatformTimer *timer = nullptr;
    
public:
    // RVCPU owns no platform devices. Attach a timer and map interrupt
    // controllers on the bus (see RVPlatform), or inject interrupts through the
    // Core set_*_interrupt/clear_*_interrupt methods.
    RVCPU();
    ~RVCPU() override;

    void init(device::Bus *bus, int flags, unsigned int coreCount) override;
    /// Attach the shared time source to every core, or nullptr for none.
    /// Call after init() and before reset() and run(). RVCPU starts and stops it
    /// around run(), so it must outlive the CPU.
    void set_platform_timer(PlatformTimer *timer);
    void reset(word_t pc) override;
    void step() override;
    void run(bool blocked=false, const word_t *breakpoints=nullptr, unsigned int n=0) override;
    void join() override;
    bool is_running() override;

    unsigned int core_count() override;
    RVCore *get_core(unsigned int coreID) override;
};

} // namespace kxemu::cpu

#endif
