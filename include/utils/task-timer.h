#ifndef __KXEMU_UTILS_TIMER_H__
#define __KXEMU_UTILS_TIMER_H__

#include <chrono>
#include <condition_variable>
#include <thread>
#include <functional>
#include <queue>
#include <unordered_set>

namespace kxemu::utils {

class TaskTimer {
private:
    std::condition_variable cv;
    std::mutex mtx;
    std::thread *timerThread;

    using task_t = std::function<void()>;
    struct Task {
        std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> timepoint;
        task_t task;
        unsigned int id;

        bool operator<(const Task &other) const {
            return this->timepoint < other.timepoint;
        }
    };
    std::priority_queue<Task> tasks;
    unsigned int counter;
    std::unordered_set<unsigned int> removedTasks;

    bool running;
    void timer_thread();

public:
    TaskTimer();
    ~TaskTimer();

    void start_thread();

    unsigned int add_task(uint64_t delay, task_t task);
    void remove_task(unsigned int id);
};

} // namespace kxemu::utils

#endif
