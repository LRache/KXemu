#include "utils/io-loop.hpp"

#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>

#include <mutex>
#include <optional>
#include <thread>

using namespace kxemu::utils;

struct IOLoop::Impl {
    asio::io_context context;
    std::optional<asio::executor_work_guard<asio::io_context::executor_type>> work;
    std::thread thread;
    std::mutex mtx; // Serializes start() and stop()
};

IOLoop::IOLoop() : impl(std::make_unique<Impl>()) {}

IOLoop::~IOLoop() {
    this->stop();
}

void IOLoop::start() {
    std::lock_guard<std::mutex> lock(impl->mtx);
    if (impl->thread.joinable()) return;
    impl->context.restart();
    // Keep run() alive while no operation is pending.
    impl->work.emplace(impl->context.get_executor());
    impl->thread = std::thread([this] { impl->context.run(); });
}

void IOLoop::stop() {
    std::lock_guard<std::mutex> lock(impl->mtx);
    if (!impl->thread.joinable()) return;
    impl->work.reset();
    impl->context.stop();
    impl->thread.join();
}

asio::io_context &IOLoop::context() {
    return impl->context;
}
