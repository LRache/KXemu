#include "utils/task-timer.hpp"
#include <chrono>
#include <thread>
#include <utility>

using namespace kxemu::utils;

TaskTimer::TaskTimer() {
    this->running = false;
    this->timerThread = nullptr;
    this->counter = 0;
}

TaskTimer::~TaskTimer() {
    this->stop_timer();
}

void TaskTimer::timer_thread() {
    std::unique_lock<std::mutex> lock(this->mtx);
    while (this->running) {
        if (this->tasks.empty()) {
            this->cv.wait(lock);
            continue;
        }
        
        auto &next = this->tasks.top();
        
        if (this->removedTasks.find(next.id) != this->removedTasks.end()) {
            this->removedTasks.erase(next.id);
            this->tasks.pop();
            continue;
        }
        
        auto now = std::chrono::high_resolution_clock::now();
        if (next.timepoint <= now) {
            // Own the callback before unlocking: producers may reorder or grow the heap.
            Task task = std::move(const_cast<Task &>(this->tasks.top()));
            this->tasks.pop();
            lock.unlock();
            task.task();
            lock.lock();
            this->removedTasks.erase(task.id);
        } else {
            const auto deadline = next.timepoint;
            this->cv.wait_until(lock, deadline);
        }
    }
}

void TaskTimer::start_timer() {
    std::lock_guard<std::mutex> lock(this->mtx);
    if (this->running) {
        return;
    }
    this->running = true;
    this->timerThread = new std::thread(&TaskTimer::timer_thread, this);
}

void TaskTimer::stop_timer() {
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        if (!this->running) {
            return;
        }
        this->running = false;
    }
    this->cv.notify_all();
    
    this->timerThread->join();
    delete this->timerThread;
    this->timerThread = nullptr;
}

unsigned int TaskTimer::add_task(uint64_t delay, task_t task) {
    std::lock_guard<std::mutex> lock(this->mtx);
    auto timepoint = std::chrono::high_resolution_clock::now() + std::chrono::nanoseconds(delay);
    unsigned int id = this->counter++;
    this->tasks.push({timepoint, task, id});
    this->cv.notify_all();
    return id;
}

void TaskTimer::remove_task(unsigned int id) {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->removedTasks.insert(id);
    this->cv.notify_all();
}
