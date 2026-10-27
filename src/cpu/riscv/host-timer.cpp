#include "cpu/riscv/host-timer.hpp"
#include "cpu/riscv/def.hpp"

#include <chrono>
#include <utility>

using namespace kxemu::cpu;

static uint64_t host_now() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

HostPlatformTimer::~HostPlatformTimer() {
    this->stop();
}

void HostPlatformTimer::start() {
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        if (this->running) return;
        this->base = host_now() - this->paused;
        this->running = true;
    }
    this->taskTimer.start_timer();
}

void HostPlatformTimer::stop() {
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        if (!this->running) return;
        this->paused = host_now() - this->base;
        this->running = false;
    }
    // Callbacks read the time, so join without holding the lock.
    this->taskTimer.stop_timer();
}

void HostPlatformTimer::reset() {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->paused = 0;
    this->base = host_now();
}

uint64_t HostPlatformTimer::uptime() {
    std::lock_guard<std::mutex> lock(this->mtx);
    return this->running ? host_now() - this->base : this->paused;
}

uint64_t HostPlatformTimer::mtime() {
    return realtime_to_mtime(this->uptime());
}

PlatformTimer::Handle HostPlatformTimer::schedule(uint64_t deadline, Callback cb) {
    // Keep the delay representable as std::chrono::nanoseconds.
    constexpr uint64_t maxDelay = uint64_t{1} << 62;
    if (deadline > realtime_to_mtime(std::numeric_limits<uint64_t>::max())) {
        return INVALID_HANDLE;
    }

    const uint64_t target = mtime_to_realtime(deadline);
    const uint64_t now = this->uptime();
    const uint64_t delay = target > now ? target - now : 0;
    if (delay > maxDelay) {
        return INVALID_HANDLE;
    }
    return this->taskTimer.add_task(delay, std::move(cb));
}

void HostPlatformTimer::cancel(Handle handle) {
    if (handle != INVALID_HANDLE) {
        this->taskTimer.remove_task(handle);
    }
}
