#include "catch-support.hpp"
#include "cpu/riscv/cpu.hpp"
#include "cpu/riscv/def.hpp"

#include "cpu/riscv/platform-timer.hpp"

#include <chrono>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#ifndef CONFIG_DIFFTEST
#error "This test requires CONFIG_DIFFTEST"
#endif

using namespace std::chrono_literals;
using kxemu::cpu::PlatformTimer;
using kxemu::cpu::RVCPU;
using kxemu::cpu::RVCore;
using kxemu::cpu::word_t;
using kxemu::device::Bus;

constexpr word_t entry = 0x80000000;
constexpr word_t stip = word_t{1} << 5;
constexpr word_t mtip = word_t{1} << 7;
constexpr unsigned wfi = 0x10500073;
constexpr unsigned nop = 0x00000013;

// A framework-driven time source: mtime moves only on advance().
class ManualTimer : public PlatformTimer {
public:
    uint64_t mtime() override {
        std::lock_guard<std::mutex> lock(this->mtx);
        return this->now;
    }

    Handle schedule(uint64_t deadline, Callback cb) override {
        std::lock_guard<std::mutex> lock(this->mtx);
        const Handle handle = this->nextHandle++;
        this->tasks.emplace(handle, Task{deadline, std::move(cb)});
        return handle;
    }

    void cancel(Handle handle) override {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->tasks.erase(handle);
    }

    // Run due callbacks outside the lock, as callbacks reschedule.
    void advance(uint64_t mtime) {
        std::vector<Callback> due;
        {
            std::lock_guard<std::mutex> lock(this->mtx);
            this->now = mtime;
            for (auto it = this->tasks.begin(); it != this->tasks.end();) {
                if (it->second.deadline <= mtime) {
                    due.push_back(std::move(it->second.cb));
                    it = this->tasks.erase(it);
                } else {
                    ++it;
                }
            }
        }
        for (auto &cb : due) cb();
    }

private:
    struct Task {
        uint64_t deadline;
        Callback cb;
    };
    std::mutex mtx;
    uint64_t now = 0;
    Handle nextHandle = 0;
    std::map<Handle, Task> tasks;
};

// A single hart with no platform devices and, by default, no timer.
struct DifftestMachine {
    Bus bus;
    RVCPU cpu;
    RVCore *core;

    explicit DifftestMachine(PlatformTimer *timer = nullptr) {
        REQUIRE_MSG(bus.add_memory_map(entry, 4096), "RAM map failed");
        cpu.init(&bus, 0, 1);
        cpu.set_platform_timer(timer);
        cpu.reset(entry);
        core = cpu.get_core(0);
    }

    void write(word_t addr, unsigned inst) {
        REQUIRE_MSG(bus.write(addr, inst, 4), "instruction write failed");
    }

    // csrrw x0/x1, csr, x2
    void csrrw(word_t addr, unsigned csr, unsigned rd) {
        write(addr, (csr << 20) | (2u << 15) | (rd << 7) | 0x1073u);
    }
};

TEST_CASE("difftest cpu maps no platform devices", "[difftest]") {
    DifftestMachine machine;
    bool valid = true;
    machine.bus.read(kxemu::cpu::ACLINT.BASE, 4, valid);
    REQUIRE_MSG(!valid, "RVCPU mapped the ACLINT");
    valid = true;
    machine.bus.read(kxemu::cpu::PLIC.BASE, 4, valid);
    REQUIRE_MSG(!valid, "RVCPU mapped the PLIC");
    REQUIRE(machine.bus.get_interrupt_controller() == nullptr);
}

TEST_CASE("difftest wfi returns immediately", "[difftest][wfi]") {
    DifftestMachine machine;
    machine.write(entry, wfi);
    machine.write(entry + 4, nop);
    // Without a bare core this would block, as no interrupt is enabled.
    auto done = std::async(std::launch::async, [&] { machine.core->step(); });
    const auto status = done.wait_for(2s);
    if (status != std::future_status::ready) machine.core->request_stop();
    REQUIRE_MSG(status == std::future_status::ready, "bare WFI blocked");
    done.wait();
    REQUIRE(machine.core->get_pc() == entry + 4);
}

TEST_CASE("difftest time CSR and stimecmp work without a timer", "[difftest][csr]") {
    DifftestMachine machine;
    // csrrw x1, time is read-only, so use csrrs x1, time, x0.
    machine.write(entry, (0xc01u << 20) | (0u << 15) | (2u << 12) | (1u << 7) | 0x73u);
    machine.core->set_gpr(1, 0x55);
    machine.core->step();
    REQUIRE_MSG(machine.core->get_pc() == entry + 4, "time read trapped");
    REQUIRE_MSG(machine.core->get_gpr(1) == 0, "time did not read zero");

    // stimecmp is only stored, and never drives STIP, even when it is already due.
#ifdef KXEMU_ISA32
    REQUIRE(machine.core->set_register("menvcfgh", word_t{1} << 31));
#else
    REQUIRE(machine.core->set_register("menvcfg", word_t{1} << 63));
#endif
    machine.csrrw(entry + 4, kxemu::cpu::CSRAddr::STIMECMP, 0);
    machine.core->set_gpr(2, 0);
    machine.core->step();
    REQUIRE(machine.core->get_register("stimecmp").value() == 0);
    REQUIRE_MSG((machine.core->get_register("mip").value() & stip) == 0, "bare stimecmp drove STIP");
}

TEST_CASE("difftest framework timer drives time and Sstc", "[difftest][csr][timer]") {
    ManualTimer timer;
    DifftestMachine machine(&timer);
    // csrrs x1, time, x0
    machine.write(entry, (0xc01u << 20) | (2u << 12) | (1u << 7) | 0x73u);
    timer.advance(0x1234);
    machine.core->step();
    REQUIRE_MSG(machine.core->get_gpr(1) == 0x1234, "time did not follow the framework timer");

#ifdef KXEMU_ISA32
    REQUIRE(machine.core->set_register("menvcfgh", word_t{1} << 31));
#else
    REQUIRE(machine.core->set_register("menvcfg", word_t{1} << 63));
#endif
    machine.csrrw(entry + 4, kxemu::cpu::CSRAddr::STIMECMP, 0);
    machine.core->set_gpr(2, 0x2000);
    machine.core->step();
    REQUIRE((machine.core->get_register("mip").value() & stip) == 0);
    timer.advance(0x1fff);
    REQUIRE_MSG((machine.core->get_register("mip").value() & stip) == 0, "STIP asserted early");
    timer.advance(0x2000);
    REQUIRE_MSG((machine.core->get_register("mip").value() & stip) != 0, "deadline did not assert STIP");
}

TEST_CASE("difftest framework injects timer interrupts", "[difftest][trap]") {
    DifftestMachine machine;
    constexpr word_t handler = entry + 0x100;
    machine.write(entry, nop);
    REQUIRE(machine.core->set_register("mtvec", handler));
    REQUIRE(machine.core->set_register("mie", mtip));
    // mstatus.MIE
    machine.csrrw(entry + 0x200, 0x300, 0);
    machine.core->set_gpr(2, word_t{1} << 3);
    machine.core->set_pc(entry + 0x200);
    machine.core->step();
    machine.core->set_pc(entry);

    machine.core->set_timer_interrupt_m();
    machine.core->step();
    REQUIRE_MSG(machine.core->get_pc() == handler, "injected MTIP did not trap");
    const word_t cause = machine.core->get_register("mcause").value();
    REQUIRE(cause == ((word_t{1} << (sizeof(word_t) * 8 - 1)) | 7));
    // The interrupted instruction did not retire, even though the framework set pc itself.
    REQUIRE_MSG(machine.core->get_register("mepc").value() == entry, "mepc is not the interrupted pc");
    REQUIRE_MSG((machine.core->get_register("mstatus").value() & (word_t{1} << 3)) == 0, "trap did not clear MIE");
}

TEST_CASE("difftest cpu runs and tears down without a timer thread", "[difftest]") {
    DifftestMachine machine;
    machine.write(entry, nop);
    machine.write(entry + 4, nop);
    machine.cpu.step();
    machine.cpu.step();
    REQUIRE(machine.core->get_pc() == entry + 8);
}
