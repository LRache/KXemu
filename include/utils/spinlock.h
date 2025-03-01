#ifndef __KXEMU_UTILS_SPINLOCK_H__
#define __KXEMU_UTILS_SPINLOCK_H__

#include <atomic>
namespace kxemu::utils {

class SpinLock {
private:
    std::atomic<bool> spinlock = false;

    public:

    void lock() {
        bool expected = false;
        while (this->spinlock.compare_exchange_weak(expected, true, std::memory_order_acquire)) {
            // Spin
        }
    }

    void unlock() {
        this->spinlock.store(false, std::memory_order_release);
    }
};

} // namespace kxemu::utils

#endif
