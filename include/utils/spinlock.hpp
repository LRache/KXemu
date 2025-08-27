#ifndef __KXEMU_UTILS_SPINLOCK_HPP__
#define __KXEMU_UTILS_SPINLOCK_HPP__

#include <atomic>
#include <thread>

namespace kxemu::utils {

class SpinLock {
private:
    std::atomic_flag spinlock = ATOMIC_FLAG_INIT;

public:
    void lock() {
        while (spinlock.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    void unlock() {
        spinlock.clear(std::memory_order_release);
    }
};

} // namespace kxemu::utils


#endif
