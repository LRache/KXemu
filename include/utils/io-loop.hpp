#ifndef __KXEMU_UTILS_IO_LOOP_HPP__
#define __KXEMU_UTILS_IO_LOOP_HPP__

#include <memory>

namespace asio {
class io_context;
}

namespace kxemu::utils {

// Runs an Asio io_context for device I/O on one background thread.
//
// Devices create their I/O objects on context() and co_spawn coroutines on it.
// Call stop() before destroying the devices: afterwards no handler runs, and
// coroutines still waiting are destroyed with the loop without being resumed.
// I/O objects created on context() must be destroyed before the loop.
class IOLoop {
public:
    IOLoop();
    IOLoop(const IOLoop &) = delete;
    IOLoop &operator=(const IOLoop &) = delete;
    ~IOLoop();

    void start();
    // Stop the context and join the thread. No handler runs after this returns.
    void stop();

    asio::io_context &context();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace kxemu::utils

#endif
