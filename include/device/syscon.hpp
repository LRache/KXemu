#ifndef __KXEMU_DEVICE_SYSCON_HPP__
#define __KXEMU_DEVICE_SYSCON_HPP__

#include "device/mmio.hpp"

#include <atomic>
#include <functional>
#include <utility>

namespace kxemu::device {

// The SiFive test finisher that QEMU's virt machine exposes at 0x100000 as
// "sifive,test1", "sifive,test0", "syscon". A write to offset 0 reports PASS,
// FAIL or RESET; the KDB layer installs the handler so the device model does
// not depend on emulator or command-line state.
class Syscon : public MMIODev {
public:
    enum class Action {
        Poweroff, // FINISHER_PASS, 0x5555
        Fail,     // FINISHER_FAIL, 0x3333
        Reset,    // FINISHER_RESET, 0x7777
    };

    using Handler = std::function<void(Action action, unsigned int code)>;

    static constexpr word_t SIZE = 0x1000;
    static constexpr word_t FINISHER_FAIL = 0x3333;
    static constexpr word_t FINISHER_PASS = 0x5555;
    static constexpr word_t FINISHER_RESET = 0x7777;

    void set_handler(Handler handler) {
        this->handler = std::move(handler);
    }

    word_t read(word_t offset, word_t size, bool &valid) override;
    bool write(word_t offset, word_t data, word_t size) override;

    const char *get_type_name() const override {
        return "syscon";
    }

private:
    Handler handler;
    // A terminal action must run once even if several harts race on the device.
    std::atomic<bool> terminalFired{false};
};

} // namespace kxemu::device

#endif
