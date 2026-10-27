#ifndef __KXEMU_CPU_RISCV_PLATFORM_TIMER_HPP__
#define __KXEMU_CPU_RISCV_PLATFORM_TIMER_HPP__

#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>

namespace kxemu::cpu {

// Shared RISC-V time base and deadline scheduler for every hart, in mtime ticks.
// The framework may supply its own implementation. Implementations must:
// - Not run cb synchronously inside schedule(): callers hold their own lock.
// - Not hold their own lock while running a callback: callbacks call back into
//   mtime(), schedule() and cancel().
// - Run no callback after stop() returns.
// Callbacks may run on any thread and may fire early, so callers must recheck.
class PlatformTimer {
public:
    using Callback = std::function<void()>;
    using Handle = unsigned int;
    static constexpr Handle INVALID_HANDLE = std::numeric_limits<Handle>::max();

    virtual ~PlatformTimer() = default;

    virtual uint64_t mtime() = 0;

    // Run cb around the time mtime reaches deadline.
    // Returns INVALID_HANDLE when the deadline can never be reached.
    virtual Handle schedule(uint64_t deadline, Callback cb) = 0;
    virtual void cancel(Handle handle) = 0;

    // Called by RVCPU when execution starts and stops.
    virtual void start() {}
    virtual void stop() {}
    // Called by the platform on reset.
    virtual void reset() {}
};

// Level-triggered comparator: the output is asserted while mtime >= compare.
// The PlatformTimer must be stopped before a comparator is destroyed.
// Lock order: the comparator mutex is taken before the core's pending lock
// (setLevel runs under it), so nothing may take a comparator or AClint mutex
// while holding the pending lock.
class TimerComparator {
public:
    using LevelFunc = std::function<void(bool asserted)>;

    void init(PlatformTimer *timer, LevelFunc setLevel);
    void set(uint64_t compare);
    // Disconnect from the output: cancel the deadline and leave the level as is.
    void disarm();
    // Disarm with the maximum compare value and deassert the output.
    void reset();
    uint64_t value();

private:
    void disarm_locked();
    void rearm_locked(bool updateLevel);
    void on_deadline(uint64_t generation);

    PlatformTimer *timer = nullptr;
    LevelFunc setLevel;

    std::mutex mtx;
    uint64_t compare = std::numeric_limits<uint64_t>::max();
    uint64_t generation = 0; // Invalidates callbacks that were already running
    PlatformTimer::Handle handle = PlatformTimer::INVALID_HANDLE;
};

} // namespace kxemu::cpu

#endif
