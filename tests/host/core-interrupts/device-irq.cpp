#include "catch-support.hpp"
#include "cpu/riscv/core.hpp"
#include "cpu/riscv/plic.hpp"
#include "device/bus.hpp"
#include "device/uart.hpp"
#include "device/virtio/block.hpp"
#include "utils/io-loop.hpp"

#include <asio/io_context.hpp>
#include <asio/post.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace std::chrono_literals;
using kxemu::cpu::RVCore;
using kxemu::cpu::word_t;
using kxemu::device::Bus;
using kxemu::device::PLIC;
using kxemu::device::Uart16650;
using kxemu::utils::IOLoop;

constexpr word_t meip = word_t{1} << 11;
constexpr word_t seip = word_t{1} << 9;

static bool wait_until(auto pred, std::chrono::milliseconds timeout = 1s) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!pred()) {
        if (std::chrono::steady_clock::now() > deadline) return false;
        std::this_thread::sleep_for(1ms);
    }
    return true;
}

// PLIC register offsets.
static constexpr word_t priority(unsigned source) { return source * 4; }
static constexpr word_t enable(unsigned context) { return 0x2000 + context * 0x80; }
static constexpr word_t threshold(unsigned context) { return 0x200000 + context * 0x1000; }
static constexpr word_t claim(unsigned context) { return threshold(context) + 4; }

// Two harts behind a PLIC, driven directly.
struct PlicFixture {
    Bus bus;
    RVCore cores[2];
    PLIC plic;

    PlicFixture() {
        for (unsigned i = 0; i < 2; i++) {
            cores[i].init(i, &bus);
            cores[i].reset(0);
        }
        RVCore *const list[] = {&cores[0], &cores[1]};
        plic.init(list);
        plic.reset();
    }

    word_t mip(unsigned hart) { return cores[hart].get_register("mip").value(); }
    word_t rd(word_t offset) {
        bool valid;
        const word_t value = plic.read(offset, 4, valid);
        REQUIRE(valid);
        return value;
    }
    void wr(word_t offset, word_t value) { REQUIRE(plic.write(offset, value, 4)); }
};

TEST_CASE("plic routes levels to contexts", "[plic]") {
    PlicFixture f;
    f.wr(priority(1), 1);
    f.wr(priority(2), 3);

    f.plic.set_irq(1, true);
    REQUIRE_MSG(f.mip(0) == 0, "disabled source raised an output");
    REQUIRE(f.rd(0x1000) == (1u << 1));

    // Contexts are fixed per hart and mode, independent of the privilege mode.
    f.wr(enable(3), 1u << 1); // hart 1, S-mode
    REQUIRE(f.mip(1) == seip);
    REQUIRE(f.mip(0) == 0);

    f.wr(threshold(3), 1);
    REQUIRE_MSG(f.mip(1) == 0, "priority must exceed the threshold");
    f.wr(threshold(3), 0);
    REQUIRE(f.mip(1) == seip);

    f.plic.set_irq(1, false);
    REQUIRE_MSG(f.mip(1) == 0, "deasserted level kept the output");
}

TEST_CASE("plic claim and complete", "[plic]") {
    PlicFixture f;
    f.wr(priority(1), 1);
    f.wr(priority(2), 3);
    f.wr(enable(0), (1u << 1) | (1u << 2));
    f.plic.set_irq(1, true);
    f.plic.set_irq(2, true);
    REQUIRE(f.mip(0) == meip);

    REQUIRE_MSG(f.rd(claim(0)) == 2, "claim did not pick the highest priority");
    REQUIRE(f.mip(0) == meip); // Source 1 is still pending.
    REQUIRE(f.rd(claim(0)) == 1);
    REQUIRE(f.mip(0) == 0);
    REQUIRE(f.rd(claim(0)) == 0);

    // A claimed source is not re-requested until completion.
    f.plic.set_irq(2, false);
    f.plic.set_irq(2, true);
    REQUIRE(f.mip(0) == 0);
    f.wr(claim(0), 2);
    REQUIRE_MSG(f.mip(0) == meip, "completion lost a still-asserted source");
    REQUIRE(f.rd(claim(0)) == 2);

    f.plic.set_irq(1, false);
    f.wr(claim(0), 1);
    REQUIRE(f.mip(0) == 0);

    f.plic.reset();
    REQUIRE_MSG(f.rd(0x1000) == (1u << 2), "reset dropped an asserted level");
}

TEST_CASE("io loop runs handlers on its thread", "[ioloop]") {
    IOLoop loop;
    loop.start();
    std::atomic<std::thread::id> ranOn{};
    asio::post(loop.context(), [&] { ranOn = std::this_thread::get_id(); });
    REQUIRE(wait_until([&] { return ranOn.load() != std::thread::id{}; }));
    REQUIRE(ranOn.load() != std::this_thread::get_id());

    loop.stop();
    std::atomic<bool> ran{false};
    asio::post(loop.context(), [&] { ran = true; });
    std::this_thread::sleep_for(20ms);
    REQUIRE_MSG(!ran, "a handler ran after stop()");
    loop.start(); // Restarting runs what was queued meanwhile.
    REQUIRE(wait_until([&] { return ran.load(); }));
    loop.stop();
}

TEST_CASE("uart socket input raises an external interrupt", "[uart][ioloop]") {
    constexpr word_t uart = 0x10000000;
    constexpr unsigned irq = 10;
    IOLoop loop; // Outlives the UART and its sockets.
    PlicFixture f;
    REQUIRE(f.bus.add_mmio_map(irq, uart, UART_LENGTH, new Uart16650));
    auto *dev = static_cast<Uart16650 *>(f.bus.match_mmio(uart)->dev);
    f.bus.set_interrupt_controller(&f.plic);
    f.wr(priority(irq), 1);
    f.wr(enable(1), 1u << irq); // hart 0, S-mode

    int peer[2];
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, peer) == 0);
    loop.start();
    REQUIRE(dev->set_fds(loop, peer[0], peer[0]));
    REQUIRE(f.bus.write(uart + 1, 0x01, 1)); // IER: received data available

    REQUIRE(::write(peer[1], "x", 1) == 1);
    REQUIRE_MSG(wait_until([&] { return f.mip(0) == seip; }), "UART input did not reach SEIP");
    bool valid;
    REQUIRE((f.bus.read(uart + 2, 1, valid) & 0x0f) == 0x04); // IIR: received data
    REQUIRE(f.rd(claim(1)) == irq);
    REQUIRE(f.bus.read(uart, 1, valid) == 'x');
    f.wr(claim(1), irq);
    REQUIRE_MSG(f.mip(0) == 0, "draining the FIFO did not lower the IRQ");

    // THR empty: raised by enabling it, acknowledged by reading IIR.
    REQUIRE(f.bus.write(uart + 1, 0x02, 1));
    REQUIRE(f.mip(0) == seip);
    REQUIRE(f.rd(claim(1)) == irq);
    REQUIRE((f.bus.read(uart + 2, 1, valid) & 0x0f) == 0x02);
    f.wr(claim(1), irq);
    REQUIRE(f.mip(0) == 0);

    // THR writes go out on the same socket.
    REQUIRE(f.bus.write(uart, 'y', 1));
    char out = 0;
    REQUIRE(::read(peer[1], &out, 1) == 1);
    REQUIRE(out == 'y');

    loop.stop();
    f.bus.set_interrupt_controller(nullptr);
    delete dev;
    close(peer[1]);
}

TEST_CASE("uart reads and writes through separate pipes", "[uart][ioloop]") {
    constexpr word_t uart = 0x10000000;
    IOLoop loop; // Outlives the UART and its fds.
    PlicFixture f;
    REQUIRE(f.bus.add_mmio_map(10, uart, UART_LENGTH, new Uart16650));
    auto *dev = static_cast<Uart16650 *>(f.bus.match_mmio(uart)->dev);

    int in[2], out[2];
    REQUIRE(pipe(in) == 0);
    REQUIRE(pipe(out) == 0);
    loop.start();
    REQUIRE(dev->set_fds(loop, in[0], out[1]));

    REQUIRE(::write(in[1], "x", 1) == 1);
    bool valid;
    REQUIRE_MSG(wait_until([&] { return (f.bus.read(uart + 5, 1, valid) & 0x01) != 0; }),
                "pipe input did not reach the FIFO");
    REQUIRE(f.bus.read(uart, 1, valid) == 'x');

    REQUIRE(f.bus.write(uart, 'y', 1));
    char c = 0;
    REQUIRE(::read(out[0], &c, 1) == 1);
    REQUIRE(c == 'y');

    loop.stop();
    delete dev;
    close(in[1]);
    close(out[0]);
}

// A virtio-blk device behind the PLIC, with one 8-entry queue in guest RAM.
struct BlockFixture : PlicFixture {
    static constexpr word_t base = 0x10001000;
    static constexpr word_t ram = 0x80000000;
    static constexpr word_t desc = ram + 0x1000;
    static constexpr word_t avail = ram + 0x2000;
    static constexpr word_t used = ram + 0x3000;
    static constexpr word_t buffers = ram + 0x4000;
    static constexpr unsigned queueNum = 8;
    static constexpr unsigned irq = 1;
    static constexpr uint64_t sectors = 16;

    std::string path;
    kxemu::device::VirtIOBlock *blk = nullptr;
    uint16_t availIdx = 0;

    BlockFixture() {
        char name[] = "/tmp/kxemu-virtio-blk-XXXXXX";
        const int fd = mkstemp(name);
        REQUIRE(fd >= 0);
        path = name;
        std::vector<uint8_t> image(sectors * 512);
        for (std::size_t i = 0; i < image.size(); i++) image[i] = pattern(i);
        REQUIRE(::write(fd, image.data(), image.size()) == (ssize_t)image.size());
        close(fd);

        REQUIRE(bus.add_memory_map(ram, 0x10000));
        REQUIRE(bus.memset(ram, 0x10000, 0));
        blk = new kxemu::device::VirtIOBlock(4);
        REQUIRE(blk->open_raw_img(path));
        REQUIRE(bus.add_mmio_map(irq, base, 0x200, blk));
        bus.set_interrupt_controller(&plic);
        wr(priority(irq), 1);
        wr(enable(1), 1u << irq); // hart 0, S-mode

        reg(0x70, 0x0f); // ACKNOWLEDGE, DRIVER, FEATURES_OK, DRIVER_OK
        reg(0x30, 0);
        reg(0x38, queueNum);
        reg(0x80, desc);
        reg(0x90, avail);
        reg(0xa0, used);
        reg(0x44, 1);
    }

    ~BlockFixture() {
        blk->quiesce();
        bus.set_interrupt_controller(nullptr);
        delete blk;
        unlink(path.c_str());
    }

    static uint8_t pattern(std::size_t offset) { return (offset * 7 + offset / 512) & 0xff; }

    void reg(word_t offset, word_t value) { REQUIRE(bus.write(base + offset, value, 4)); }
    word_t reg(word_t offset) {
        bool valid;
        const word_t value = bus.read(base + offset, 4, valid);
        REQUIRE(valid);
        return value;
    }
    uint8_t *mem(word_t addr) { return (uint8_t *)bus.get_ptr(addr); }

    struct Desc { word_t addr; uint32_t len; bool write; };
    // Chain descs from slot first, publish the head and notify.
    void submit(unsigned first, const std::vector<Desc> &chain) {
        for (unsigned i = 0; i < chain.size(); i++) {
            const word_t d = desc + (first + i) * 16;
            const bool next = i + 1 < chain.size();
            REQUIRE(bus.write(d, chain[i].addr, 8));
            REQUIRE(bus.write(d + 8, chain[i].len, 4));
            REQUIRE(bus.write(d + 12, (next ? 1 : 0) | (chain[i].write ? 2 : 0), 2));
            REQUIRE(bus.write(d + 14, first + i + 1, 2));
        }
        REQUIRE(bus.write(avail + 4 + (availIdx % queueNum) * 2, first, 2));
        availIdx++;
        REQUIRE(bus.write(avail + 2, availIdx, 2));
        reg(0x50, 0);
    }
    void header(word_t addr, uint32_t type, uint64_t sector) {
        REQUIRE(bus.write(addr, type, 4));
        REQUIRE(bus.write(addr + 4, 0, 4));
        REQUIRE(bus.write(addr + 8, sector, 8));
    }
    uint16_t used_idx() {
        return std::atomic_ref<uint16_t>(*(uint16_t *)mem(used + 2)).load(std::memory_order_acquire);
    }
    bool wait_used(uint16_t idx) { return wait_until([&] { return used_idx() == idx; }); }
    uint32_t used_id(unsigned i) { return *(uint32_t *)mem(used + 4 + i * 8); }
    uint32_t used_len(unsigned i) { return *(uint32_t *)mem(used + 8 + i * 8); }
};

TEST_CASE("virtio-blk serves multi-segment requests asynchronously", "[virtio][irq]") {
    BlockFixture f;
    const word_t hdr = f.buffers, data = f.buffers + 0x100, status = f.buffers + 0x80;
    REQUIRE(f.reg(0x60) == 0);

    SECTION("read") {
        // The data spans two descriptors; the status follows the data.
        f.header(hdr, 0, 2);
        f.submit(0, {{hdr, 16, false}, {data, 512, true}, {data + 512, 512, true}, {status, 1, true}});
        REQUIRE(f.wait_used(1));
        REQUIRE(f.used_id(0) == 0);
        REQUIRE(f.used_len(0) == 1025);
        REQUIRE(*f.mem(status) == 0);
        for (std::size_t i = 0; i < 1024; i++) {
            REQUIRE(f.mem(data)[i] == BlockFixture::pattern(2 * 512 + i));
        }
        REQUIRE_MSG(f.mip(0) == seip, "completion did not raise the IRQ");
        REQUIRE(f.reg(0x60) == 1);
        f.reg(0x64, 1);
        REQUIRE_MSG(f.mip(0) == 0, "interrupt ACK did not lower the IRQ");
    }

    SECTION("write, flush and the last sector") {
        std::memset(f.mem(data), 0xa5, 512);
        f.header(hdr, 1, BlockFixture::sectors - 1);
        f.submit(0, {{hdr, 16, false}, {data, 512, false}, {status, 1, true}});
        REQUIRE(f.wait_used(1));
        REQUIRE(*f.mem(status) == 0);
        REQUIRE(f.used_len(0) == 1);

        f.header(hdr + 0x20, 4, 0);
        f.submit(3, {{hdr + 0x20, 16, false}, {status + 1, 1, true}});
        REQUIRE(f.wait_used(2));
        REQUIRE(*f.mem(status + 1) == 0);

        f.blk->quiesce();
        const int fd = open(f.path.c_str(), O_RDONLY);
        uint8_t back[512];
        REQUIRE(pread(fd, back, sizeof(back), (BlockFixture::sectors - 1) * 512) == 512);
        close(fd);
        for (auto byte : back) REQUIRE(byte == 0xa5);
    }

    SECTION("errors complete with a status") {
        f.header(hdr, 0, BlockFixture::sectors); // Past the end
        f.submit(0, {{hdr, 16, false}, {data, 512, true}, {status, 1, true}});
        REQUIRE(f.wait_used(1));
        REQUIRE(*f.mem(status) == 1); // IOERR

        f.header(hdr + 0x20, 99, 0); // Unknown type
        f.submit(3, {{hdr + 0x20, 16, false}, {status + 1, 1, true}});
        REQUIRE(f.wait_used(2));
        REQUIRE(*f.mem(status + 1) == 2); // UNSUPP
    }
}

TEST_CASE("virtio-blk reset and quiesce wait for in-flight requests", "[virtio]") {
    BlockFixture f;
    // Two descriptors per request, so the chains do not overlap. The status byte
    // shares the last descriptor with the data.
    for (unsigned i = 0; i < 4; i++) {
        const word_t hdr = f.buffers + i * 0x400;
        f.header(hdr, 0, i);
        f.submit(i * 2, {{hdr, 16, false}, {hdr + 0x10, 512 + 1, true}});
    }
    SECTION("quiesce") {
        f.blk->quiesce();
        REQUIRE(f.used_idx() == 4);
    }
    SECTION("reset") {
        f.reg(0x70, 0);
        REQUIRE(f.used_idx() == 4);
        REQUIRE(f.reg(0x70) == 0);
        REQUIRE_MSG(f.mip(0) == 0, "reset kept the IRQ asserted");
    }
    // Completion order is free, but every head comes back once.
    std::vector<uint32_t> heads;
    for (unsigned i = 0; i < 4; i++) heads.push_back(f.used_id(i));
    std::sort(heads.begin(), heads.end());
    REQUIRE(heads == std::vector<uint32_t>{0, 2, 4, 6});
}
