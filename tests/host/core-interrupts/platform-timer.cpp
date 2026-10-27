#include "catch-support.hpp"
#include "cpu/riscv/aclint.hpp"
#include "cpu/riscv/core.hpp"
#include "cpu/riscv/def.hpp"
#include "cpu/riscv/host-timer.hpp"
#include "cpu/riscv/plic.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;
using kxemu::cpu::RVCore;
using kxemu::cpu::word_t;
using kxemu::device::AClint;
using kxemu::device::Bus;
using kxemu::device::PLIC;
using kxemu::cpu::HostPlatformTimer;
using kxemu::cpu::PlatformTimer;
using kxemu::cpu::TimerComparator;

constexpr word_t entry = 0x80000000;
constexpr uint64_t never = std::numeric_limits<uint64_t>::max();
constexpr uint64_t ticks_2ms = 20000;
constexpr word_t mtip = word_t{1} << 7;
constexpr word_t stip = word_t{1} << 5;

// Records comparator output and the mtime observed when it was asserted.
struct LevelProbe {
    std::mutex mtx;
    std::condition_variable cv;
    bool level = false;
    unsigned asserts = 0;
    uint64_t assertedAt = 0;

    TimerComparator::LevelFunc func(PlatformTimer &timer) {
        return [this, &timer](bool asserted) {
            std::lock_guard<std::mutex> lock(this->mtx);
            this->level = asserted;
            if (asserted) {
                this->asserts++;
                this->assertedAt = timer.mtime();
            }
            this->cv.notify_all();
        };
    }

    bool wait_asserted(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(this->mtx);
        return this->cv.wait_for(lock, timeout, [this] { return this->level; });
    }
};

// Declared in this order so the timer stops before the comparator is destroyed.
struct ComparatorFixture {
    LevelProbe probe;
    TimerComparator comparator;
    HostPlatformTimer timer;

    ComparatorFixture() {
        this->comparator.init(&this->timer, this->probe.func(this->timer));
    }
};

// A single hart wired to the ACLINT without RVCPU.
struct TimerMachine {
    Bus bus;
    HostPlatformTimer timer;
    PLIC plic;
    RVCore core;
    AClint aclint;

    TimerMachine() {
        REQUIRE_MSG(this->bus.add_memory_map(entry, 4096), "RAM map failed");
        this->core.init(0, &this->bus, &this->timer);
        RVCore *const cores[] = {&this->core};
        this->plic.init(cores);
        this->plic.connect_to_bus(&this->bus);
        this->bus.set_interrupt_controller(&this->plic);
        this->aclint.init(cores, &this->timer);
        this->core.reset(entry);
    }

    ~TimerMachine() {
        this->timer.stop();
    }

    word_t mip() {
        return this->core.get_register("mip").value();
    }

    void set_stce(bool enabled) {
#ifdef KXEMU_ISA32
        REQUIRE_MSG(this->core.set_register("menvcfgh", enabled ? word_t{1} << 31 : 0), "menvcfgh write failed");
#else
        REQUIRE_MSG(this->core.set_register("menvcfg", enabled ? word_t{1} << 63 : 0), "menvcfg write failed");
#endif
    }

    // Execute csrrc x0, mip, x2 in M-mode.
    void guest_clear_mip(word_t mask) {
        const uint32_t csrrc = (uint32_t(kxemu::cpu::CSRAddr::MIP) << 20) | (2u << 15) | 0x3073u;
        REQUIRE_MSG(this->bus.write(entry, csrrc, 4), "csrrc write failed");
        this->core.set_gpr(2, mask);
        this->core.set_pc(entry);
        this->core.step();
        REQUIRE_MSG(this->core.get_pc() == entry + 4, "csrrc mip trapped");
    }

    void write_mtimecmp(uint64_t value) {
#ifdef KXEMU_ISA64
        REQUIRE_MSG(this->aclint.write(0x4000, value, 8), "MTIMECMP write failed");
#else
        REQUIRE_MSG(this->aclint.write(0x4004, 0xffffffff, 4), "MTIMECMP high write failed");
        REQUIRE_MSG(this->aclint.write(0x4000, uint32_t(value), 4), "MTIMECMP low write failed");
        REQUIRE_MSG(this->aclint.write(0x4004, uint32_t(value >> 32), 4), "MTIMECMP high write failed");
#endif
    }

    bool wait_mip(word_t mask, std::chrono::milliseconds timeout) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (this->mip() & mask) return true;
            std::this_thread::sleep_for(100us);
        }
        return (this->mip() & mask) != 0;
    }
};

TEST_CASE("mtime pauses while stopped and rewinds on reset", "[timer]") {
    HostPlatformTimer timer;
    REQUIRE(timer.mtime() == 0);

    timer.start();
    std::this_thread::sleep_for(2ms);
    timer.stop();
    const uint64_t paused = timer.mtime();
    REQUIRE_MSG(paused > 0, "mtime did not advance while running");
    std::this_thread::sleep_for(2ms);
    REQUIRE_MSG(timer.mtime() == paused, "mtime advanced while stopped");

    timer.start();
    std::this_thread::sleep_for(1ms);
    REQUIRE_MSG(timer.mtime() > paused, "mtime did not resume from the paused value");
    timer.stop();

    timer.reset();
    REQUIRE(timer.mtime() == 0);
}

TEST_CASE("comparator output follows mtime >= compare", "[timer]") {
    ComparatorFixture f;
    // The timer is stopped, so mtime stays at zero.
    f.comparator.set(0);
    REQUIRE(f.probe.level);
    f.comparator.set(1);
    REQUIRE_FALSE(f.probe.level);
    f.comparator.set(0);
    REQUIRE(f.probe.level);
    f.comparator.reset();
    REQUIRE_FALSE(f.probe.level);
    REQUIRE(f.comparator.value() == never);
}

TEST_CASE("comparator asserts from the timer thread at its deadline", "[timer]") {
    ComparatorFixture f;
    f.timer.start();
    const uint64_t compare = f.timer.mtime() + ticks_2ms;
    f.comparator.set(compare);
    REQUIRE_FALSE(f.probe.level);
    REQUIRE_MSG(f.probe.wait_asserted(2000ms), "deadline did not assert the comparator");
    REQUIRE(f.probe.assertedAt >= compare);
}

TEST_CASE("rewriting the compare value cancels the old deadline", "[timer]") {
    ComparatorFixture f;
    f.timer.start();
    f.comparator.set(f.timer.mtime() + ticks_2ms);
    f.comparator.set(never);
    std::this_thread::sleep_for(20ms);
    REQUIRE(f.probe.asserts == 0);
}

TEST_CASE("deadline that fires early after a pause rearms instead of asserting", "[timer]") {
    ComparatorFixture f;
    f.timer.start();
    const uint64_t compare = f.timer.mtime() + ticks_2ms;
    f.comparator.set(compare);
    f.timer.stop();
    // The host deadline passes while mtime is paused.
    std::this_thread::sleep_for(10ms);
    f.timer.start();
    REQUIRE_MSG(f.probe.wait_asserted(2000ms), "rearmed deadline did not assert the comparator");
    REQUIRE(f.probe.asserts == 1);
    REQUIRE(f.probe.assertedAt >= compare);
}

TEST_CASE("MTIMECMP raises and clears MTIP without device polling", "[timer][aclint]") {
    TimerMachine machine;
    machine.timer.start();
    machine.write_mtimecmp(machine.timer.mtime() + ticks_2ms);
    REQUIRE_FALSE(machine.mip() & mtip);
    REQUIRE_MSG(machine.wait_mip(mtip, 2000ms), "MTIMECMP deadline did not raise MTIP");

    machine.write_mtimecmp(never);
    REQUIRE_MSG((machine.mip() & mtip) == 0, "MTIMECMP rewrite did not clear MTIP");
}

TEST_CASE("MTIMECMP reads back both halves", "[timer][aclint]") {
    TimerMachine machine;
    constexpr uint64_t value = 0x123456789abcdef0ULL;
    machine.write_mtimecmp(value);
    bool valid = false;
#ifdef KXEMU_ISA64
    REQUIRE(machine.aclint.read(0x4000, 8, valid) == value);
    REQUIRE(valid);
#else
    REQUIRE(machine.aclint.read(0x4000, 4, valid) == uint32_t(value));
    REQUIRE(valid);
    REQUIRE(machine.aclint.read(0x4004, 4, valid) == uint32_t(value >> 32));
    REQUIRE(valid);
#endif
}

TEST_CASE("stimecmp raises STIP without device polling", "[timer][csr]") {
    TimerMachine machine;
    machine.set_stce(true);
    machine.timer.start();
    const uint64_t compare = machine.timer.mtime() + ticks_2ms;
    REQUIRE(machine.core.set_register("stimecmp", word_t(compare)));
    REQUIRE_FALSE(machine.mip() & stip);
    REQUIRE_MSG(machine.wait_mip(stip, 2000ms), "stimecmp deadline did not raise STIP");

    REQUIRE(machine.core.set_register("stimecmp", ~word_t{0}));
    REQUIRE_MSG((machine.mip() & stip) == 0, "stimecmp rewrite did not clear STIP");
}

TEST_CASE("menvcfg.STCE connects stimecmp to STIP", "[timer][csr]") {
    TimerMachine machine;
    // The timer is stopped, so mtime stays at zero and stimecmp=0 is due.
    REQUIRE(machine.core.set_register("stimecmp", 0));
    REQUIRE_MSG((machine.mip() & stip) == 0, "stimecmp drove STIP while STCE=0");

    machine.set_stce(true);
    REQUIRE_MSG((machine.mip() & stip) != 0, "enabling STCE did not apply the pending stimecmp");
    machine.guest_clear_mip(stip);
    REQUIRE_MSG((machine.mip() & stip) != 0, "guest mip write changed STIP while STCE=1");

    // Disabling STCE leaves STIP as is and makes it software-writable again.
    machine.set_stce(false);
    REQUIRE((machine.mip() & stip) != 0);
    machine.guest_clear_mip(stip);
    REQUIRE_MSG((machine.mip() & stip) == 0, "guest mip write could not clear STIP while STCE=0");
    REQUIRE(machine.core.set_register("stimecmp", 0));
    REQUIRE_MSG((machine.mip() & stip) == 0, "stimecmp drove STIP while STCE=0");
}
