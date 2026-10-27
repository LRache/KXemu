#include "cpu/riscv/platform-timer.hpp"

#include <utility>

using namespace kxemu::cpu;

void TimerComparator::init(PlatformTimer *timer, LevelFunc setLevel) {
    // Cancel any deadline still queued on the previous timer.
    this->disarm();
    this->timer = timer;
    this->setLevel = std::move(setLevel);
}

void TimerComparator::set(uint64_t compare) {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->compare = compare;
    this->rearm_locked(true);
}

void TimerComparator::disarm() {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->disarm_locked();
}

void TimerComparator::reset() {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->disarm_locked();
    this->setLevel(false);
}

uint64_t TimerComparator::value() {
    std::lock_guard<std::mutex> lock(this->mtx);
    return this->compare;
}

void TimerComparator::disarm_locked() {
    this->generation++;
    if (this->timer != nullptr) this->timer->cancel(this->handle);
    this->handle = PlatformTimer::INVALID_HANDLE;
    this->compare = std::numeric_limits<uint64_t>::max();
}

void TimerComparator::rearm_locked(bool updateLevel) {
    this->generation++;
    if (this->timer != nullptr) this->timer->cancel(this->handle);
    this->handle = PlatformTimer::INVALID_HANDLE;
    if (this->timer == nullptr) return;

    const bool due = this->timer->mtime() >= this->compare;
    if (due || updateLevel) {
        this->setLevel(due);
    }
    if (!due) {
        const uint64_t generation = this->generation;
        this->handle = this->timer->schedule(this->compare, [this, generation] {
            this->on_deadline(generation);
        });
    }
}

void TimerComparator::on_deadline(uint64_t generation) {
    std::lock_guard<std::mutex> lock(this->mtx);
    // A newer compare value or reset replaced this deadline.
    if (generation != this->generation) return;
    this->handle = PlatformTimer::INVALID_HANDLE;
    this->rearm_locked(false);
}
