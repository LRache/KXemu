#include "utils/task-timer.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>

using namespace std::chrono_literals;
using kxemu::utils::TaskTimer;

TEST_CASE("queue growth during a callback preserves callbacks", "[timer]") {
    std::promise<void> entered, release, finished;
    auto enteredFuture = entered.get_future();
    auto releaseFuture = release.get_future().share();
    auto finishedFuture = finished.get_future();
    std::atomic<unsigned> count{0};
    auto captured = std::make_shared<unsigned>(42);
    unsigned observed = 0;
    TaskTimer timer;
    timer.add_task(0, [&, captured] {
        entered.set_value();
        releaseFuture.wait();
        // This capture must remain valid even if the queue's vector reallocates.
        observed = *captured;
    });
    timer.start_timer();
    const auto started = enteredFuture.wait_for(2s);
    constexpr unsigned tasks = 2048;
    for (unsigned i = 0; i < tasks; ++i) {
        timer.add_task(0, [&] {
            if (++count == tasks) finished.set_value();
        });
    }
    release.set_value();
    const auto completed = finishedFuture.wait_for(2s);
    timer.stop_timer();
    REQUIRE(started == std::future_status::ready);
    REQUIRE(completed == std::future_status::ready);
    REQUIRE(observed == 42);
    REQUIRE(count == tasks);
}

TEST_CASE("cancelled tasks do not execute or discard the next task", "[timer]") {
    std::atomic<unsigned> cancelledCalls{0};
    std::promise<void> finished;
    auto finishedFuture = finished.get_future();
    TaskTimer timer;
    const auto id = timer.add_task(0, [&] { ++cancelledCalls; });
    timer.add_task(0, [&] { finished.set_value(); });
    timer.remove_task(id);
    timer.start_timer();
    const auto completed = finishedFuture.wait_for(2s);
    timer.stop_timer();
    REQUIRE(completed == std::future_status::ready);
    REQUIRE(cancelledCalls == 0);
}

TEST_CASE("idle timer can stop and restart without missed notifications", "[timer]") {
    TaskTimer timer;
    for (unsigned i = 0; i < 100; ++i) {
        timer.start_timer();
        timer.stop_timer();
    }
    // Repeated stop and destruction after stop are safe.
    timer.stop_timer();
}
