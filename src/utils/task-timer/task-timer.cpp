#include "utils/task-timer.h"

using namespace kxemu::utils;

TaskTimer::TaskTimer() {
    this->running = false;
    this->timerThread = nullptr;
    this->counter = 0;
}

TaskTimer::~TaskTimer() {
    if (this->running) {
        this->running = false;
        this->cv.notify_all();
        this->timerThread->join();
        delete this->timerThread;
    }
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
            this->tasks.pop();
            this->removedTasks.erase(next.id);
            continue;
        }
        
        auto now = std::chrono::high_resolution_clock::now();
        if (next.timepoint <= now) {
            lock.unlock();
            next.task();
            lock.lock();
            this->tasks.pop();
        } else {
            this->cv.wait_until(lock, next.timepoint);
        }
    }
}

void TaskTimer::start_thread() {
    if (this->running) {
        return;
    }
    this->running = true;
    this->timerThread = new std::thread(&TaskTimer::timer_thread, this);
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
