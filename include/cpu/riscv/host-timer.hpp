#ifndef __KXEMU_CPU_RISCV_HOST_TIMER_HPP__
#define __KXEMU_CPU_RISCV_HOST_TIMER_HPP__

#include "cpu/riscv/platform-timer.hpp"
#include "utils/task-timer.hpp"

#include <cstdint>
#include <mutex>

namespace kxemu::cpu {

// PlatformTimer driven by the host steady clock.
// mtime only advances while the timer runs: it pauses across stop()/start()
// and moves backwards only on reset().
class HostPlatformTimer : public PlatformTimer {
public:
    ~HostPlatformTimer() override;

    void start() override;
    // Joins the timer thread, so no callback runs after this returns.
    void stop() override;
    // Rewind mtime to zero. Pending deadlines are kept and fire early.
    void reset() override;

    uint64_t uptime(); // nanoseconds
    uint64_t mtime() override;

    Handle schedule(uint64_t deadline, Callback cb) override;
    void cancel(Handle handle) override;

private:
    std::mutex mtx;
    bool running = false;
    uint64_t base = 0;   // Host time at which uptime was zero, while running
    uint64_t paused = 0; // Frozen uptime, while stopped
    utils::TaskTimer taskTimer;
};

} // namespace kxemu::cpu

#endif
