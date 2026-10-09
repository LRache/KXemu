#include "catch-support.hpp"
#include "cpu/riscv/cpu.hpp"
#include "cpu/riscv/def.hpp"

#include <catch2/generators/catch_generators.hpp>

#include <chrono>
#include <atomic>
#include <barrier>
#include <future>
#include <memory>
#include <thread>

using namespace std::chrono_literals;
using kxemu::cpu::RVCPU;
using kxemu::cpu::word_t;
using kxemu::device::Bus;


static word_t read_csr(kxemu::cpu::RVCSR &csr, unsigned int addr, bool &valid) {
    const auto result = csr.modify_csr(addr, 0, kxemu::cpu::RVCSR::ModifyOp::Read);
    valid = result.valid;
    return result.oldValue;
}

static word_t read_csr(kxemu::cpu::RVCSR &csr, unsigned int addr) {
    bool valid;
    return read_csr(csr, addr, valid);
}

static kxemu::cpu::RVCSR::ModifyResult write_csr(kxemu::cpu::RVCSR &csr, unsigned int addr, word_t value) {
    return csr.modify_csr(addr, value, kxemu::cpu::RVCSR::ModifyOp::Write, false);
}

static word_t read_checked_csr(kxemu::cpu::RVCSR &csr, unsigned int addr) {
    bool valid;
    const word_t value = read_csr(csr, addr, valid);
    REQUIRE_MSG(valid, "implemented CSR became invalid");
    return value;
}

static void write_checked_csr(kxemu::cpu::RVCSR &csr, unsigned int addr, word_t value) {
    REQUIRE_MSG(write_csr(csr, addr, value).valid, "implemented CSR write became invalid");
}

constexpr word_t entry = 0x80000000;
constexpr word_t software = word_t{1} << 3;
constexpr word_t timer = word_t{1} << 7;

struct Machine {
    Bus bus;
    RVCPU cpu;

    Machine() {
        REQUIRE_MSG(bus.add_memory_map(entry, 4096), "RAM map failed");
        cpu.init(&bus, 0, 1);
        cpu.reset(entry);
        REQUIRE_MSG(bus.write(entry, 0x10500073, 4), "WFI write failed");
        REQUIRE_MSG(bus.write(entry + 4, 0x00000013, 4), "NOP write failed");
        auto core = cpu.get_core(0);
        core->set_register("mie", software);
        set_mstatus(0);
    }

    void set_mstatus(word_t value) {
        // Program mstatus through the guest CSR path.
        auto core = cpu.get_core(0);
        REQUIRE_MSG(bus.write(entry + 0x200, (0x300u << 20) | (31u << 15) | 0x1073, 4),
                    "CSR instruction write failed");
        core->set_gpr(31, value);
        core->set_pc(entry + 0x200);
        core->step();
        core->set_pc(entry);
    }
};

TEST_CASE("trap status after raw writes", "[wfi][trap]") {
    Machine machine;
    auto core = machine.cpu.get_core(0);
    constexpr word_t handler = entry + 0x100;
    REQUIRE_MSG(machine.bus.write(handler, 0x00000013, 4), "handler NOP write failed");
    REQUIRE_MSG(machine.bus.write(handler + 4, 0x30200073, 4), "MRET write failed");
    core->set_register("mtvec", handler);
    core->set_register("mstatus", software); // Exercise the raw CSR write path.
    auto first = std::async(std::launch::async, [&] { core->step(); });
    REQUIRE_MSG(first.wait_for(20ms) == std::future_status::timeout, "WFI did not wait");
    core->set_software_interrupt_m();
    REQUIRE_MSG(first.wait_for(1s) == std::future_status::ready, "first IRQ did not wake WFI");
    first.get();
    REQUIRE_MSG(core->get_pc() == handler, "raw mstatus write did not enable interrupt");
    REQUIRE_MSG((core->get_register("mstatus").value() & software) == 0, "trap did not clear MIE");
    core->step();
    REQUIRE_MSG(core->get_pc() == handler + 4, "pending IRQ retrapped inside handler");
    core->clear_software_interrupt_m();
    core->step();
    REQUIRE_MSG((core->get_register("mstatus").value() & software) != 0, "MRET did not restore MIE");
    core->set_pc(entry);
    auto second = std::async(std::launch::async, [&] { core->step(); });
    REQUIRE_MSG(second.wait_for(20ms) == std::future_status::timeout, "returned WFI did not wait");
    core->set_software_interrupt_m();
    REQUIRE_MSG(second.wait_for(1s) == std::future_status::ready, "second IRQ did not wake WFI");
    second.get();
    REQUIRE_MSG(core->get_pc() == handler, "MRET-restored MIE did not enable interrupt");
}

TEST_CASE("trap and float csr storage", "[csr]") {
    using namespace kxemu::cpu;
    RVCSR csr;
    csr.reset();
    const CSRAddr trap[] = {CSRAddr::MTVEC, CSRAddr::STVEC, CSRAddr::MEPC,
        CSRAddr::SEPC, CSRAddr::MCAUSE, CSRAddr::SCAUSE, CSRAddr::MTVAL,
        CSRAddr::STVAL, CSRAddr::MTINST, CSRAddr::STINST, CSRAddr::MTVAL2};
    for (unsigned i = 0; i < std::size(trap); ++i) {
        CAPTURE(i);
        const auto result = write_csr(csr, trap[i], i + 1);
        REQUIRE_MSG(result.valid, "trap CSR guest write failed");
        REQUIRE_MSG(result.effect == RVCSR::WriteEffect::None, "trap CSR guest write failed");
        REQUIRE_MSG(read_checked_csr(csr, trap[i]) == i + 1, "trap CSR stored value lost");
    }
    write_checked_csr(csr, CSRAddr::MEPC, 99);
    REQUIRE_MSG(read_csr(csr, CSRAddr::MEPC) == 99, "trap CSR write failed");
    write_checked_csr(csr, CSRAddr::FCSR, 0x1e3);
    REQUIRE_MSG(read_csr(csr, CSRAddr::FCSR) == 0xe3, "fcsr write did not split or mask floating state");
    REQUIRE_MSG(read_csr(csr, CSRAddr::FFLAGS) == 3, "fcsr write did not split or mask floating state");
    REQUIRE_MSG(read_csr(csr, CSRAddr::FRM) == 7, "fcsr write did not split or mask floating state");
    REQUIRE_MSG(write_csr(csr, CSRAddr::FFLAGS, 0x1f).effect == RVCSR::WriteEffect::None,
                "fflags write produced unrelated effect");
    REQUIRE_MSG(read_csr(csr, CSRAddr::FCSR) == 0xff, "fflags write damaged frm");
    REQUIRE_MSG(read_csr(csr, CSRAddr::FRM) == 7, "fflags write damaged frm");
    REQUIRE_MSG(write_csr(csr, CSRAddr::FRM, 2).effect == RVCSR::WriteEffect::None,
                "frm write produced unrelated effect");
    REQUIRE_MSG(read_csr(csr, CSRAddr::FCSR) == 0x5f, "frm write damaged fflags");
    REQUIRE_MSG(read_csr(csr, CSRAddr::FRM) == 2, "frm write damaged fflags");
    REQUIRE_MSG(write_csr(csr, CSRAddr::FCSR, 0xa5).effect == RVCSR::WriteEffect::None,
                "fcsr write produced unrelated effect");
    REQUIRE_MSG(read_csr(csr, CSRAddr::FFLAGS) == 5, "fcsr write did not update views");
    REQUIRE_MSG(read_csr(csr, CSRAddr::FRM) == 5, "fcsr write did not update views");
    write_checked_csr(csr, CSRAddr::FFLAGS, 0xff);
    REQUIRE_MSG(read_csr(csr, CSRAddr::FCSR) == 0xbf, "fflags write damaged frm or missed the mask");
    REQUIRE_MSG(read_csr(csr, CSRAddr::FRM) == 5, "fflags write damaged frm or missed the mask");
    write_checked_csr(csr, CSRAddr::FRM, 0xff);
    REQUIRE_MSG(read_csr(csr, CSRAddr::FCSR) == 0xff,
                "frm write damaged fflags or missed the mask");
    csr.reset();
    for (auto addr : trap) REQUIRE_MSG(read_csr(csr, addr) == 0, "trap CSR reset retained value");
    REQUIRE_MSG(read_csr(csr, CSRAddr::FCSR) == 0, "floating CSR reset retained value");
    REQUIRE_MSG(read_csr(csr, CSRAddr::FFLAGS) == 0, "floating CSR reset retained value");
    REQUIRE_MSG(read_csr(csr, CSRAddr::FRM) == 0, "floating CSR reset retained value");
}

TEST_CASE("remaining csr storage", "[csr]") {
    using namespace kxemu::cpu;
    RVCSR csr;
    csr.init(7, [] { return uint64_t{1000000}; });
    csr.privMode = PrivMode::MACHINE;
    csr.reset();
    const struct { CSRAddr addr; word_t reset; } registers[] = {
        {CSRAddr::MVENDORID, KXEMU_VENDORID}, {CSRAddr::MARCHID, KXEMU_ARCHID},
        {CSRAddr::MIMPID, 0}, {CSRAddr::MHARTID, 7}, {CSRAddr::MCFGPTR, 0},
        {CSRAddr::MISA, kxemu::cpu::csr::MISA()},
        {CSRAddr::MCNTEN, 0}, {CSRAddr::SCNTEN, 0},
        {CSRAddr::MENVCFG, 0}, {CSRAddr::MSECCFG, 0},
        {CSRAddr::MSCRATCH, 0}, {CSRAddr::SSCRATCH, 0},
        {CSRAddr::SATP, 0}, {CSRAddr::STIMECMP, 0},
        {CSRAddr::CYCLE, 0},
#ifdef KXEMU_ISA32
        {CSRAddr::MSTATUSH, 0}, {CSRAddr::MENVCFGH, 0}, {CSRAddr::MSECCFGH, 0},
        {CSRAddr::STIMECMPH, 0}, {CSRAddr::CYCLEH, 0},
#endif
    };
    for (const auto &reg : registers) {
        CAPTURE(reg.addr);
        REQUIRE_MSG(read_checked_csr(csr, reg.addr) == reg.reset, "CSR reset value changed");
        if (!csr_read_only(reg.addr) && reg.addr != CSRAddr::SATP) {
            write_checked_csr(csr, reg.addr, ~word_t{0} - reg.addr);
        }
    }
    for (const auto &reg : registers) {
        CAPTURE(reg.addr);
        const word_t expected = csr_read_only(reg.addr) || reg.addr == CSRAddr::SATP
            ? reg.reset : ~word_t{0} - reg.addr;
        REQUIRE_MSG(read_checked_csr(csr, reg.addr) == expected, "CSR write lost bits or aliased another CSR");
        if (!csr_read_only(reg.addr) && reg.addr != CSRAddr::SATP) {
            const auto result = write_csr(csr, reg.addr, 0x12345678);
            REQUIRE_MSG(result.valid, "guest CSR write lost bits");
            REQUIRE_MSG(read_checked_csr(csr, reg.addr) == 0x12345678, "guest CSR write lost bits");
        }
    }
    csr::Satp satpInput(~word_t{0});
#ifdef KXEMU_ISA64
    satpInput.set_mode(csr::Satp::SV39);
#endif
    const auto satpWrite = write_csr(csr, CSRAddr::SATP, satpInput);
    REQUIRE_MSG(satpWrite.valid, "SATP guest write missed ASID mask or effect");
    REQUIRE_MSG(satpWrite.effect == RVCSR::WriteEffect::VmTranslate, "SATP guest write missed ASID mask or effect");
    REQUIRE_MSG(csr::Satp(read_csr(csr, CSRAddr::SATP)).asid() == 0, "SATP guest write missed ASID mask or effect");
    csr.reset();
    for (const auto &reg : registers) {
        CAPTURE(reg.addr);
        REQUIRE_MSG(read_checked_csr(csr, reg.addr) == reg.reset, "CSR reset retained a written value");
    }
    csr.init(9, [] { return uint64_t{1000000}; });
    csr.reset();
    REQUIRE_MSG(read_csr(csr, CSRAddr::MHARTID) == 9, "reset did not use the new hart ID");
#ifdef KXEMU_ISA64
    for (unsigned addr : {0x310u, 0x31au, 0x757u, 0x15du, 0xc80u, 0xc81u}) {
        CAPTURE(addr);
        bool valid;
        read_csr(csr, addr, valid);
        REQUIRE_MSG(!valid, "RV32-only CSR became readable on RV64");
        if (!csr_read_only(addr)) REQUIRE_MSG(!write_csr(csr, addr, 1).valid,
                                            "RV32-only CSR became writable on RV64");
    }
#endif
}

TEST_CASE("time csr access", "[csr]") {
    using namespace kxemu::cpu;
    RVCSR csr;
    constexpr uint64_t uptime = 1000000;
    unsigned reads = 0;
    csr.init(0, [&] { ++reads; return uptime; });
    csr.reset();
    for (auto mode : {PrivMode::MACHINE, PrivMode::SUPERVISOR, PrivMode::USER}) {
        csr.privMode = mode;
        for (unsigned enables = 0; enables < 8; ++enables) {
            CAPTURE(mode, enables);
            write_checked_csr(csr, CSRAddr::MCNTEN, enables & 1 ? 2 : 0);
            write_checked_csr(csr, CSRAddr::SCNTEN, enables & 2 ? 2 : 0);
#ifdef KXEMU_ISA32
            write_checked_csr(csr, CSRAddr::MENVCFGH, enables & 4 ? word_t{1} << 31 : 0);
#else
            write_checked_csr(csr, CSRAddr::MENVCFG, enables & 4 ? word_t{1} << 63 : 0);
#endif
            const bool allowed = mode == PrivMode::MACHINE ||
                (enables & 1 && (mode == PrivMode::SUPERVISOR ? enables & 4 : enables & 2));
            const auto previousReads = reads;
            bool valid;
            const auto value = read_csr(csr, CSRAddr::TIME, valid);
            REQUIRE_MSG(valid == allowed, "TIME access permissions changed");
            REQUIRE_MSG(reads == previousReads + unsigned(allowed), "denied TIME read queried uptime");
            if (allowed) REQUIRE_MSG(value == word_t(realtime_to_mtime(uptime)), "TIME value changed");
#ifdef KXEMU_ISA32
            const auto high = read_csr(csr, CSRAddr::TIMEH, valid);
            REQUIRE_MSG(valid == allowed, "TIMEH access permissions changed");
            if (allowed) REQUIRE_MSG(high == word_t(uint64_t{realtime_to_mtime(uptime)} >> 32), "TIMEH value changed");
#endif
        }
    }
}

TEST_CASE("csr derived values and effects", "[csr]") {
    using namespace kxemu::cpu;
    RVCSR csr;
    csr.reset();
    const auto satpWrite = write_csr(csr, CSRAddr::SATP, 0);
    REQUIRE_MSG(satpWrite.valid, "satp write did not report translation effect");
    REQUIRE_MSG(satpWrite.effect == RVCSR::WriteEffect::VmTranslate, "satp write did not report translation effect");
    REQUIRE_MSG(read_csr(csr, CSRAddr::SATP) == 0, "satp write did not report translation effect");

    const auto timerWrite = write_csr(csr, CSRAddr::STIMECMP, 0x12345678);
    REQUIRE_MSG(timerWrite.valid, "stimecmp write did not report timer effect");
    REQUIRE_MSG(timerWrite.effect == RVCSR::WriteEffect::STimer, "stimecmp write did not report timer effect");
#ifdef KXEMU_ISA32
    const auto timerHighWrite = write_csr(csr, CSRAddr::STIMECMPH, 0x9abcdef0);
    REQUIRE_MSG(timerHighWrite.valid, "stimecmph write did not update 64-bit timer value");
    REQUIRE_MSG(timerHighWrite.effect == RVCSR::WriteEffect::STimer,
                "stimecmph write did not update 64-bit timer value");
    REQUIRE_MSG(csr.stimecmp_value() == 0x9abcdef012345678ULL, "stimecmph write did not update 64-bit timer value");
#else
    REQUIRE_MSG(csr.stimecmp_value() == 0x12345678,
                "stimecmp value did not reflect the CSR write");
#endif
}

TEST_CASE("guest stimecmp rearms timer", "[csr][timer]") {
    using namespace kxemu::cpu;
    Machine machine;
    auto core = machine.cpu.get_core(0);
    constexpr word_t stip = word_t{1} << InterruptCode::TIMER_S;
    constexpr word_t pc = entry + 0x300;
    const unsigned csrrw = (unsigned(CSRAddr::STIMECMP) << 20) | (2u << 15) | 0x1073u;
    REQUIRE_MSG(machine.bus.write(pc, csrrw, 4), "stimecmp test instruction write failed");
    REQUIRE_MSG(machine.bus.write(pc + 4, 0x00000013, 4), "stimecmp test instruction write failed");
    core->set_timer_interrupt_s();
    core->set_gpr(2, 0);
    core->set_pc(pc);
    core->step();
    REQUIRE_MSG((core->get_register("mip").value() & stip) == 0,
                "guest stimecmp write did not clear the old timer interrupt");
    core->step();
    REQUIRE_MSG((core->get_register("mip").value() & stip) != 0,
                "guest stimecmp write did not arm the ACLINT timer");
    REQUIRE_MSG(core->set_register("stimecmp", ~word_t{0}), "debug stimecmp write failed");
    REQUIRE_MSG((core->get_register("mip").value() & stip) == 0,
                "debug stimecmp write did not rearm the timer");
}

TEST_CASE("wfi", "[wfi]") {
    Machine machine;
    auto core = machine.cpu.get_core(0);
    // A masked timer is visible immediately but must not finish WFI.
    core->set_timer_interrupt_m();
    REQUIRE_MSG(core->get_register("mip").value() == timer, "producer did not update mip immediately");
    auto step = std::async(std::launch::async, [&] { core->step(); });
    REQUIRE_MSG(step.wait_for(20ms) == std::future_status::timeout, "masked IRQ woke WFI");
    core->set_software_interrupt_m();
    REQUIRE_MSG(step.wait_for(1s) == std::future_status::ready, "enabled IRQ did not wake WFI");
    step.get();
    REQUIRE_MSG(core->get_pc() == entry + 4, "WFI did not advance with global MIE off");
    REQUIRE_MSG((core->get_register("mip").value() & (software | timer)) == (software | timer),
                "wait returned without injecting IRQs");

    core->clear_software_interrupt_m();
    core->step();
    REQUIRE_MSG((core->get_register("mip").value() & software) == 0, "IRQ clear not synchronized");
    REQUIRE_MSG((core->get_register("mip").value() & timer) != 0, "IRQ clear damaged another bit");
}

TEST_CASE("pending and delegated", "[wfi][irq]") {
    Machine machine;
    auto core = machine.cpu.get_core(0);
    // Delegation/global enables affect trap entry, not the local WFI predicate.
    core->set_register("mideleg", software);
    core->set_software_interrupt_m();
    auto step = std::async(std::launch::async, [&] { core->step(); });
    REQUIRE_MSG(step.wait_for(1s) == std::future_status::ready, "preexisting delegated IRQ lost");
    step.get();
    REQUIRE_MSG(core->get_pc() == entry + 4, "delegated IRQ incorrectly trapped to M");
}

TEST_CASE("trap after wfi", "[wfi][trap]") {
    Machine machine;
    auto core = machine.cpu.get_core(0);
    machine.set_mstatus(software); // mstatus.MIE
    core->set_register("mtvec", entry + 0x100);
    auto step = std::async(std::launch::async, [&] { core->step(); });
    REQUIRE_MSG(step.wait_for(20ms) == std::future_status::timeout, "WFI failed to wait");
    core->set_software_interrupt_m();
    REQUIRE_MSG(step.wait_for(1s) == std::future_status::ready, "IRQ failed to resume WFI");
    step.get();
    REQUIRE_MSG(core->get_pc() == entry + 0x100, "trap delayed after WFI");
    REQUIRE_MSG(core->get_register("mepc").value() == entry + 4, "wrong WFI return PC");
}

TEST_CASE("stop and reset", "[wfi][stop]") {
    Machine machine;
    auto core = machine.cpu.get_core(0);
    kxemu::cpu::Core<word_t> *coreInterface = core;
    core->set_register("mie", 0);
    auto run = std::async(std::launch::async, [&] { core->run(); });
    REQUIRE_MSG(run.wait_for(20ms) == std::future_status::timeout, "empty WFI failed to wait");
    coreInterface->request_stop();
    REQUIRE_MSG(run.wait_for(1s) == std::future_status::ready, "stop failed to release run");
    run.get();
    REQUIRE_MSG(!core->is_running(), "stopped core still running");
    REQUIRE_MSG(core->stop_requested(), "stop flag was consumed");
    machine.cpu.reset(entry + 4);
    REQUIRE_MSG(!core->stop_requested(), "reset did not clear stop flag");
    core->step();
    REQUIRE_MSG(core->get_pc() == entry + 8, "reset did not make core runnable");

    machine.cpu.reset(entry);
    auto step = std::async(std::launch::async, [&] { core->step(); });
    REQUIRE_MSG(step.wait_for(20ms) == std::future_status::timeout, "reset WFI failed to wait");
    coreInterface->request_stop();
    REQUIRE_MSG(step.wait_for(1s) == std::future_status::ready, "stop failed to release step");
    step.get();
    REQUIRE_MSG(!core->is_running(), "stopped single-step core still running");
}

TEST_CASE("concurrent irq updates", "[irq][concurrency]") {
    Machine machine;
    auto core = machine.cpu.get_core(0);
    constexpr word_t unrelated = word_t{1} << 1;
    core->set_register("mip", unrelated);
    std::thread first([&] { core->set_software_interrupt_m(); });
    std::thread second([&] { core->set_timer_interrupt_m(); });
    first.join();
    second.join();
    core->set_pc(entry + 4);
    core->step();
    REQUIRE_MSG(core->get_register("mip").value() == (unrelated | software | timer),
                "concurrent producers lost an IRQ or unrelated bit");

    for (unsigned i = 0; i < 1000; ++i) {
        core->set_software_interrupt_m();
        core->clear_software_interrupt_m();
        core->wake();
    }
    core->set_pc(entry + 4);
    core->step();
    REQUIRE_MSG(core->get_register("mip").value() == (unrelated | timer),
                "coalesced IRQ updates did not retain final level");

    // A bare wake must not manufacture an enabled IRQ or finish WFI.
    core->set_pc(entry);
    auto step = std::async(std::launch::async, [&] { core->step(); });
    core->wake();
    REQUIRE_MSG(step.wait_for(20ms) == std::future_status::timeout, "bare wake finished WFI");
    core->set_software_interrupt_m();
    REQUIRE_MSG(step.wait_for(1s) == std::future_status::ready, "IRQ after bare wake was lost");
    step.get();

    // Race IRQ publication against entry to WFI repeatedly.
    std::barrier race(2);
    std::thread producer([&] {
        for (unsigned i = 0; i < 1000; ++i) {
            race.arrive_and_wait();
            core->set_software_interrupt_m();
            race.arrive_and_wait();
        }
    });
    for (unsigned i = 0; i < 1000; ++i) {
        CAPTURE(i);
        core->clear_software_interrupt_m();
        core->set_pc(entry + 4);
        core->step();
        core->set_pc(entry);
        race.arrive_and_wait();
        core->step();
        REQUIRE_MSG(core->get_pc() == entry + 4, "IRQ race failed to complete WFI");
        REQUIRE_MSG((core->get_register("mip").value() & software) != 0,
                    "WFI completed without IRQ injection");
        race.arrive_and_wait();
    }
    producer.join();
}

TEST_CASE("hart isolation", "[irq][stop]") {
    // Separate machines avoid sharing legacy polled devices between test threads.
    Machine first;
    Machine second;
    auto firstCore = first.cpu.get_core(0);
    auto secondCore = second.cpu.get_core(0);
    firstCore->set_software_interrupt_m();
    firstCore->step();
    auto step = std::async(std::launch::async, [&] { secondCore->step(); });
    REQUIRE_MSG(step.wait_for(20ms) == std::future_status::timeout, "IRQ leaked between cores");
    secondCore->request_stop();
    REQUIRE_MSG(step.wait_for(1s) == std::future_status::ready, "isolated hart stop failed");
    step.get();
    REQUIRE_MSG(!firstCore->stop_requested(), "stop flag leaked between cores");
}

TEST_CASE("polled source", "[wfi][irq]") {
    // A legacy source that publishes only when Bus::update() polls it.
    struct PolledSource : kxemu::device::MMIODev {
        using mmio_word_t = kxemu::device::word_t;
        kxemu::cpu::RVCore *core;
        std::atomic<bool> pending{false};
        explicit PolledSource(kxemu::cpu::RVCore *core) : core(core) {}
        mmio_word_t read(mmio_word_t, mmio_word_t, bool &valid) override {
            valid = false;
            return 0;
        }
        bool write(mmio_word_t, mmio_word_t, mmio_word_t) override { return false; }
        void update() override {
            if (pending.exchange(false)) core->set_timer_interrupt_m();
        }
    };

    Machine machine;
    auto core = machine.cpu.get_core(0);
    PolledSource source(core);
    REQUIRE_MSG(machine.bus.add_mmio_map(0x10000000, 4096, &source), "poll source map failed");
    core->set_register("mie", timer);
    auto step = std::async(std::launch::async, [&] { core->step(); });
    REQUIRE_MSG(step.wait_for(20ms) == std::future_status::timeout, "WFI failed to wait for poll source");
    source.pending.store(true);
    REQUIRE_MSG(step.wait_for(1s) == std::future_status::ready, "polled source no longer serviced");
    step.get();
    REQUIRE_MSG((core->get_register("mip").value() & timer) != 0, "polled IRQ not injected");
}

TEST_CASE("cpu teardown", "[stop]") {
    const bool explicitJoin = GENERATE(false, true);
    CAPTURE(explicitJoin);
    Bus bus;
    REQUIRE_MSG(bus.add_memory_map(entry, 4096), "RAM map failed");
    REQUIRE_MSG(bus.write(entry, 0x10500073, 4), "WFI write failed");
    std::unique_ptr<kxemu::cpu::CPU<word_t>> cpu = std::make_unique<RVCPU>();
    cpu->request_stop(); // An uninitialized CPU has no cores to stop.
    cpu->init(&bus, 0, 2);
    cpu->reset(entry);
    cpu->run(false);
    // Destruction must stop and join harts sleeping with mie == 0.
    // Assertions stay on the test thread, so the worker only reports hart state.
    auto teardown = std::async(std::launch::async, [cpu = std::move(cpu), explicitJoin]() mutable {
        std::pair<bool, bool> running{false, false};
        if (explicitJoin) {
            cpu->request_stop();
            cpu->request_stop(); // Repeated requests are harmless.
            cpu->join();
            cpu->join();
            running = {cpu->get_core(0)->is_running(), cpu->get_core(1)->is_running()};
        }
        cpu.reset();
        return running;
    });
    REQUIRE_MSG(teardown.wait_for(1s) == std::future_status::ready, "CPU destructor failed to join WFI");
    const auto [hart0Running, hart1Running] = teardown.get();
    REQUIRE_MSG(!hart0Running, "CPU stop missed hart 0");
    REQUIRE_MSG(!hart1Running, "CPU stop missed hart 1");
}

TEST_CASE("single core cpu stop", "[stop]") {
    Machine machine;
    kxemu::cpu::CPU<word_t> *cpu = &machine.cpu;
    auto run = std::async(std::launch::async, [&] { cpu->run(true); });
    REQUIRE_MSG(run.wait_for(20ms) == std::future_status::timeout, "CPU WFI failed to wait");
    cpu->request_stop();
    REQUIRE_MSG(run.wait_for(1s) == std::future_status::ready, "CPU stop did not release blocking run");
    run.get(); // This caller owns the execution thread, not CPU::join().
    REQUIRE_MSG(!cpu->is_running(), "single-core CPU still running after stop");
    cpu->reset(entry + 4);
    cpu->step();
    REQUIRE_MSG(cpu->get_core(0)->get_pc() == entry + 8, "CPU reset did not clear stop");
}

TEST_CASE("irq racing guest csr", "[irq][csr][concurrency]") {
    Machine machine;
    auto core = machine.cpu.get_core(0);
    constexpr word_t ssip = word_t{1} << kxemu::cpu::InterruptCode::SOFTWARE_S;
    constexpr word_t stip = word_t{1} << 5;
    const word_t pc = entry + 0x100;
    core->set_register("mie", 0);
    core->set_gpr(2, ssip);
    REQUIRE_MSG(machine.bus.write(pc, (0x344u << 20) | (2u << 15) | (2u << 12) | (3u << 7) | 0x73u, 4),
                "CSRRS write failed");
    std::barrier race(2);
    std::thread producer([&] {
        for (unsigned i = 0; i < 1000; ++i) {
            race.arrive_and_wait();
            core->set_timer_interrupt_s();
            race.arrive_and_wait();
        }
    });
    for (unsigned i = 0; i < 1000; ++i) {
        CAPTURE(i);
        core->set_register("mip", 0);
        core->set_pc(pc);
        race.arrive_and_wait();
        core->step();
        race.arrive_and_wait();
        REQUIRE_MSG(core->get_register("mip").value() == (ssip | stip), "guest CSRRS lost concurrent IRQ");
        REQUIRE_MSG((core->get_gpr(3) == 0 || core->get_gpr(3) == stip), "CSRRS returned inconsistent old value");
    }
    producer.join();
}

TEST_CASE("expanded interrupt csrs", "[csr][irq]") {
    using namespace kxemu::cpu;
    RVCSR csr;
    csr.init(7, [] { return uint64_t{0}; });
    csr.reset();
    const CSRAddr exceptionDeleg[] = {CSRAddr::MEDELEG,
#ifdef KXEMU_ISA32
        CSRAddr::MEDELEGH,
#endif
    };
    for (const auto addr : exceptionDeleg) {
        CAPTURE(addr);
        REQUIRE_MSG(read_csr(csr, addr) == 0, "exception delegation reset changed");
        const word_t value = ~word_t{0};
        const auto result = write_csr(csr, addr, value);
        REQUIRE_MSG(result.valid, "exception delegation guest write lost bits");
        REQUIRE_MSG(result.effect == RVCSR::WriteEffect::None, "exception delegation guest write lost bits");
        REQUIRE_MSG(read_checked_csr(csr, addr) == value, "exception delegation guest write lost bits");
        write_checked_csr(csr, addr, 42);
        REQUIRE_MSG(read_checked_csr(csr, addr) == 42,
                    "exception delegation write lost value");
    }
#ifdef KXEMU_ISA64
    constexpr unsigned int medeleghAddr = 0x312;
    bool highValid = true;
    read_csr(csr, medeleghAddr, highValid);
    REQUIRE_MSG(!highValid, "RV32 exception delegation CSR became valid on RV64");
    REQUIRE_MSG(!write_csr(csr, medeleghAddr, 1).valid, "RV32 exception delegation CSR became valid on RV64");
#endif
    constexpr word_t xl = kxemu::cpu::csr::MStatus::XL_VALUE;
    constexpr word_t uxl = xl & kxemu::cpu::csr::MStatus::SSTATUS_MASK;
    REQUIRE_MSG(read_csr(csr, CSRAddr::MSTATUS) == xl, "mstatus reset changed");
    REQUIRE_MSG(write_csr(csr, CSRAddr::MSTATUS, 0x180a).effect == RVCSR::WriteEffect::None,
                "mstatus write produced unrelated effect");
    REQUIRE_MSG(read_csr(csr, CSRAddr::SSTATUS) == (uxl | 2), "sstatus view missed mstatus write");
    REQUIRE_MSG(write_csr(csr, CSRAddr::SSTATUS, 0).effect == RVCSR::WriteEffect::None,
                "sstatus write produced unrelated effect");
    REQUIRE_MSG(read_csr(csr, CSRAddr::MSTATUS) == (xl | 0x1808), "sstatus write damaged machine fields");
    write_checked_csr(csr, CSRAddr::SSTATUS, 2);
    REQUIRE_MSG(read_checked_csr(csr, CSRAddr::MSTATUS) == (xl | 0x180a), "supervisor view write failed");
    constexpr word_t stip = word_t{1} << InterruptCode::TIMER_S;
    write_csr(csr, CSRAddr::MIE, timer | stip | (word_t{1} << 20));
    REQUIRE_MSG(read_checked_csr(csr, CSRAddr::MIE) == (timer | stip), "mie write mask changed");
    REQUIRE_MSG(read_csr(csr, CSRAddr::SIE) == stip, "sie view missed mie write");
    write_csr(csr, CSRAddr::SIE, 0);
    REQUIRE_MSG(read_checked_csr(csr, CSRAddr::MIE) == timer, "sie clear damaged machine enable");
    write_checked_csr(csr, CSRAddr::SIE, stip);
    REQUIRE_MSG(read_checked_csr(csr, CSRAddr::MIE) == (timer | stip), "sie write did not update mie");
    write_csr(csr, CSRAddr::MIDELEG, stip);
    REQUIRE_MSG(read_checked_csr(csr, CSRAddr::MIDELEG) == stip, "mideleg stored value missed write");
    write_csr(csr, CSRAddr::MSCRATCH, 42);
    REQUIRE_MSG(read_csr(csr, CSRAddr::MSCRATCH) == 42, "scratch CSR regressed");
    bool valid = true;
    read_csr(csr, 0x7bf, valid);
    REQUIRE_MSG(!valid, "unknown CSR became valid");
    REQUIRE_MSG(!write_csr(csr, 0x7bf, 1).valid, "unknown CSR became valid");
    csr.reset();
    REQUIRE_MSG(read_checked_csr(csr, CSRAddr::MIE) == 0, "expanded CSR reset failed");
    REQUIRE_MSG(read_checked_csr(csr, CSRAddr::MIDELEG) == 0, "expanded CSR reset failed");
    REQUIRE_MSG(read_csr(csr, CSRAddr::MSTATUS) == xl, "expanded CSR reset failed");
    for (const auto addr : exceptionDeleg) {
        CAPTURE(addr);
        REQUIRE_MSG(read_csr(csr, addr) == 0, "exception delegation retained stale reset value");
    }
    REQUIRE_MSG(read_csr(csr, CSRAddr::SIE) == 0, "supervisor views retained stale reset values");
    REQUIRE_MSG(read_csr(csr, CSRAddr::SSTATUS) == uxl, "supervisor views retained stale reset values");
    REQUIRE_MSG(read_csr(csr, CSRAddr::MHARTID) == 7, "hart ID reset value lost");
    REQUIRE_MSG(write_csr(csr, CSRAddr::MSTATUS, 0).valid, "mstatus write after reset failed");
}

TEST_CASE("pmpcfg storage", "[csr][pmp]") {
    using namespace kxemu::cpu;
    RVCSR csr;
    for (unsigned int i = 0; i < 64; ++i) {
        CAPTURE(i);
        const auto addr = static_cast<CSRAddr>(CSRAddr::PMPADDR0 + i);
        const word_t value = ~word_t{0} - i;
        REQUIRE_MSG(write_csr(csr, addr, value).valid, "pmpaddr guest write mask changed");
        REQUIRE_MSG(read_csr(csr, addr) == word_t(csr::PMPAddr(value)), "pmpaddr guest write mask changed");
    }
    for (unsigned int i = 0; i < 64; ++i) {
        CAPTURE(i);
        const auto addr = static_cast<CSRAddr>(CSRAddr::PMPADDR0 + i);
        REQUIRE_MSG(read_checked_csr(csr, addr) == word_t(csr::PMPAddr(~word_t{0} - i)),
                    "pmpaddr registers alias each other");
    }
    csr.reset();
    for (unsigned int i = 0; i < 16; ++i) {
        CAPTURE(i);
        const unsigned int addr = CSRAddr::PMPCFG0 + i;
        bool valid = false;
        const word_t initial = read_csr(csr, addr, valid);
#ifdef KXEMU_ISA64
        if (i % 2 != 0) {
            REQUIRE_MSG(!valid, "odd pmpcfg register became valid on RV64");
            REQUIRE_MSG(!write_csr(csr, addr, 7).valid, "odd pmpcfg register became valid on RV64");
            continue;
        }
#endif
        REQUIRE_MSG(valid, "pmpcfg initial state changed");
        REQUIRE_MSG(initial == 0, "pmpcfg initial state changed");
        // Keep address matching OFF while checking the whole XLEN-width word.
        word_t value = 0;
        for (unsigned int j = 0; j < sizeof(word_t); ++j) {
            value |= word_t{0xe0u | ((i + j) % 8)} << (j * 8);
        }
        const auto result = write_csr(csr, addr, value);
        REQUIRE_MSG(result.valid, "pmpcfg guest write lost value or produced unrelated effect");
        REQUIRE_MSG(result.effect == RVCSR::WriteEffect::None,
                    "pmpcfg guest write lost value or produced unrelated effect");
        REQUIRE_MSG(read_csr(csr, addr) == value, "pmpcfg guest write lost value or produced unrelated effect");
    }
    for (unsigned int i = 0; i < 16; ++i) {
        CAPTURE(i);
#ifdef KXEMU_ISA64
        if (i % 2 != 0) continue;
#endif
        const auto addr = static_cast<CSRAddr>(CSRAddr::PMPCFG0 + i);
        word_t value = 0;
        for (unsigned int j = 0; j < sizeof(word_t); ++j) {
            value |= word_t{0xe0u | ((i + j) % 8)} << (j * 8);
        }
        REQUIRE_MSG(read_checked_csr(csr, addr) == value, "pmpcfg registers alias each other");
        write_checked_csr(csr, addr, i + 1);
        REQUIRE_MSG(read_csr(csr, addr) == i + 1, "pmpcfg rewrite lost value");
    }
    for (unsigned int i = 0; i < 64; ++i) {
        CAPTURE(i);
        const auto addr = static_cast<CSRAddr>(CSRAddr::PMPADDR0 + i);
        const word_t value = ~word_t{0} - i;
        REQUIRE_MSG(write_csr(csr, addr, value).valid, "pmpaddr guest write mask changed");
        REQUIRE_MSG(read_csr(csr, addr) == word_t(csr::PMPAddr(value)), "pmpaddr guest write mask changed");
    }
    for (unsigned int i = 0; i < 64; ++i) {
        CAPTURE(i);
        const auto addr = static_cast<CSRAddr>(CSRAddr::PMPADDR0 + i);
        REQUIRE_MSG(read_checked_csr(csr, addr) == word_t(csr::PMPAddr(~word_t{0} - i)),
                    "pmpaddr registers alias each other");
    }
    csr.reset();
    for (unsigned int i = 0; i < 16; ++i) {
        CAPTURE(i);
#ifdef KXEMU_ISA64
        if (i % 2 != 0) continue;
#endif
        REQUIRE_MSG(read_csr(csr, CSRAddr::PMPCFG0 + i) == 0,
                    "pmpcfg retained stale reset value");
    }
    for (unsigned int i = 0; i < 64; ++i) {
        CAPTURE(i);
        REQUIRE_MSG(read_csr(csr, CSRAddr::PMPADDR0 + i) == 0, "pmpaddr reset failed");
    }
    bool valid = true;
    read_csr(csr, CSRAddr::PMPADDR0 + 64, valid);
    REQUIRE_MSG(!valid, "address above pmpaddr became valid");
    REQUIRE_MSG(!write_csr(csr, CSRAddr::PMPADDR0 + 64, 0).valid, "address above pmpaddr became valid");
    read_csr(csr, CSRAddr::PMPCFG0 - 1, valid);
    REQUIRE_MSG(!valid, "address below pmpcfg became valid");
    REQUIRE_MSG(!write_csr(csr, CSRAddr::PMPCFG0 - 1, 0).valid, "address below pmpcfg became valid");
    REQUIRE_MSG(write_csr(csr, CSRAddr::PMPADDR0, 0x1000).valid, "pmpaddr storage regressed");
    REQUIRE_MSG(read_csr(csr, CSRAddr::PMPADDR0) == 0x1000, "pmpaddr storage regressed");
}

TEST_CASE("pending csr hint", "[csr][irq]") {
    using namespace kxemu::cpu;
    RVCSR csr;
    csr.reset();
    REQUIRE_MSG(!csr.has_pending_interrupt(), "reset left pending hint set");
    constexpr word_t ssip = word_t{1} << InterruptCode::SOFTWARE_S;
    constexpr word_t stip = word_t{1} << InterruptCode::TIMER_S;
    write_checked_csr(csr, CSRAddr::MIP, stip);
    REQUIRE_MSG(csr.has_pending_interrupt(), "mip write missed hint");
    const auto result = csr.modify_csr(CSRAddr::MIP, ssip, RVCSR::ModifyOp::Write);
    REQUIRE_MSG(result.valid, "CSR exchange returned wrong old value");
    REQUIRE_MSG(result.oldValue == stip, "CSR exchange returned wrong old value");
    REQUIRE_MSG(read_checked_csr(csr, CSRAddr::MIP) == ssip, "CSR exchange wrote wrong pending bits");
    REQUIRE_MSG(read_checked_csr(csr, CSRAddr::SIP) == ssip, "sip is not a mip view");
    write_checked_csr(csr, CSRAddr::MIP, ssip | stip);
    write_checked_csr(csr, CSRAddr::SIP, 0);
    REQUIRE_MSG(csr.has_pending_interrupt(), "clearing sip hid a timer interrupt");
    write_checked_csr(csr, CSRAddr::MIP, 0);
    REQUIRE_MSG(!csr.has_pending_interrupt(), "clearing last pending bit left hint set");
    write_checked_csr(csr, CSRAddr::MIP, ssip);
    REQUIRE_MSG(csr.has_pending_interrupt(), "guest mip write missed hint");
    write_checked_csr(csr, CSRAddr::SIP, 0);
    REQUIRE_MSG(!csr.has_pending_interrupt(), "sip write missed hint");
    write_checked_csr(csr, CSRAddr::SIP, ssip);
    REQUIRE_MSG(csr.has_pending_interrupt(), "guest sip write missed hint");
    csr.reset();
    REQUIRE_MSG(!csr.has_pending_interrupt(), "reset did not clear nonzero hint");
}

TEST_CASE("debug csr writes", "[csr]") {
    using namespace kxemu::cpu;
    Machine machine;
    auto core = machine.cpu.get_core(0);
    constexpr word_t ssip = word_t{1} << InterruptCode::SOFTWARE_S;
    constexpr word_t stip = word_t{1} << InterruptCode::TIMER_S;
    constexpr word_t seip = word_t{1} << InterruptCode::EXTERNAL_S;
    constexpr word_t reserved = word_t{1} << 20;

    REQUIRE_MSG(core->set_register("mie", timer | reserved), "debug mie write applied guest mask");
    REQUIRE_MSG(core->get_register("mie").value() == (timer | reserved), "debug mie write applied guest mask");
    REQUIRE_MSG(core->set_register("sie", ssip), "debug sie write changed its masking behavior");
    REQUIRE_MSG(core->get_register("mie").value() == (timer | ssip), "debug sie write changed its masking behavior");
    REQUIRE_MSG(core->set_register("mstatus", 0x1808), "debug sstatus write damaged machine fields");
    REQUIRE_MSG(core->set_register("sstatus", 2), "debug sstatus write damaged machine fields");
    REQUIRE_MSG(core->get_register("mstatus").value() == 0x180a, "debug sstatus write damaged machine fields");
    csr::Satp debugSatp(~word_t{0});
#ifdef KXEMU_ISA64
    debugSatp.set_mode(csr::Satp::SV39);
#endif
    REQUIRE_MSG(core->set_register("satp", debugSatp), "debug satp write changed raw ASID or PPN bits");
    REQUIRE_MSG(core->get_register("satp").value() == word_t(debugSatp),
                "debug satp write changed raw ASID or PPN bits");
#ifdef KXEMU_ISA64
    REQUIRE_MSG(!core->set_register("satp", ~word_t{0}),
                "debug satp write accepted unsupported MODE or damaged old value");
    REQUIRE_MSG(core->get_register("satp").value() == word_t(debugSatp),
                "debug satp write accepted unsupported MODE or damaged old value");
#endif
    REQUIRE_MSG(core->set_register("satp", 0), "debug satp restore failed");
    REQUIRE_MSG(core->set_register("mip", timer | stip | seip | ssip), "debug mip write applied guest mask");
    REQUIRE_MSG(core->get_register("mip").value() == (timer | stip | seip | ssip),
                "debug mip write applied guest mask");
    REQUIRE_MSG(core->set_register("sip", 0), "debug sip write damaged read-only pending bits");
    REQUIRE_MSG(core->get_register("mip").value() == (timer | stip | seip),
                "debug sip write damaged read-only pending bits");

    // Debug MIP writes must update the hint used by interrupt scanning.
    constexpr word_t handler = entry + 0x100;
    core->set_register("mtvec", handler);
    core->set_register("mie", software);
    core->set_register("mstatus", software);
    core->set_register("mip", 0);
    core->set_pc(entry + 4);
    core->step();
    REQUIRE_MSG(core->get_pc() == entry + 8, "cleared debug mip still delivered an interrupt");
    core->set_register("mip", software);
    core->set_pc(entry + 4);
    core->step();
    REQUIRE_MSG(core->get_register("mcause").value() == word_t(csr::MCause(InterruptCode::SOFTWARE_M)),
                "debug mip write did not enable interrupt scanning");
    REQUIRE_MSG((core->get_register("mstatus").value() & software) == 0,
                "debug mip write did not enable interrupt scanning");
}

TEST_CASE("guest pending csr instructions", "[csr][irq]") {
    // Exercise all six instruction variants against both pending CSR views.
    for (unsigned addr : {0x344u, 0x144u}) {
        for (unsigned funct : {1u, 2u, 3u, 5u, 6u, 7u}) {
            CAPTURE(addr, funct);
            Machine machine;
            auto core = machine.cpu.get_core(0);
            constexpr word_t ssip = word_t{1} << kxemu::cpu::InterruptCode::SOFTWARE_S;
            constexpr word_t stip = word_t{1} << 5;
            const word_t initial = timer | stip | (funct == 3 || funct == 7 ? ssip : 0);
            core->set_register("mip", initial);
            core->set_register("mie", 0);
            core->set_gpr(2, ssip);
            const word_t pc = entry + 0x100;
            const unsigned source = funct >= 5 ? static_cast<unsigned>(ssip) : 2u;
            REQUIRE_MSG(machine.bus.write(pc, (addr << 20) | (source << 15) | (funct << 12) | (3u << 7) | 0x73u, 4),
                        "pending CSR instruction write failed");
            core->set_pc(pc);
            core->step();
            const word_t expectedOld = addr == 0x344 ? initial : initial & (ssip | stip);
            REQUIRE_MSG(core->get_gpr(3) == expectedOld, "pending CSR instruction returned wrong old value");
            word_t expected = initial;
            if (funct == 1 || funct == 5) expected = timer | ssip | (addr == 0x144 ? stip : 0);
            else if (funct == 2 || funct == 6) expected |= ssip;
            else expected &= ~ssip;
            REQUIRE_MSG(core->get_register("mip").value() == expected, "pending CSR instruction updated wrong bits");
        }
    }
}

TEST_CASE("satp validity and writes", "[csr][satp]") {
    using namespace kxemu::cpu;
    RVCSR csr;
    csr.reset();
#ifdef KXEMU_ISA64
    constexpr unsigned modeShift = 60;
    constexpr unsigned asidShift = 44;
    constexpr word_t asidMask = 0xffff;
    constexpr unsigned modeCount = 16;
    constexpr auto pagedMode = csr::Satp::SV48;
#else
    constexpr unsigned modeShift = 31;
    constexpr unsigned asidShift = 22;
    constexpr word_t asidMask = 0x1ff;
    constexpr unsigned modeCount = 2;
    constexpr auto pagedMode = csr::Satp::SV32;
#endif
    const word_t initial = (word_t{pagedMode} << modeShift) | 0x12345;
    const csr::Satp allAsids(initial | (asidMask << asidShift));
    REQUIRE_MSG(allAsids.asid() == asidMask, "SATP ASID getter lost implemented field bits");
    auto fields = allAsids;
    fields.set_asid(0);
    REQUIRE_MSG(word_t(fields) == initial, "clearing SATP ASID changed MODE/PPN or retained high ASID bits");
    fields.set_asid(~word_t{0});
    REQUIRE_MSG(word_t(fields) == word_t(allAsids), "SATP ASID setter used incorrect width");

    for (unsigned mode = 0; mode < modeCount; ++mode) {
        CAPTURE(mode);
        const word_t raw = (word_t{mode} << modeShift) | (asidMask << asidShift) | 0x6789;
        const csr::Satp candidate(raw);
#ifdef KXEMU_ISA64
        const bool supported = mode == 0 || mode == 8 || mode == 9 || mode == 10;
#else
        const bool supported = true;
#endif
        REQUIRE_MSG(word_t(candidate) == raw, "SATP constructor changed raw bits or valid() misclassified MODE");
        REQUIRE_MSG(candidate.valid() == supported, "SATP constructor changed raw bits or valid() misclassified MODE");
        for (bool readOld : {false, true}) {
            CAPTURE(readOld);
            write_checked_csr(csr, CSRAddr::SATP, initial);
            // Bare is programmed with zero remaining fields, as required of software.
            const word_t input = mode == 0 ? 0 : raw;
            const auto result = csr.modify_csr(CSRAddr::SATP, input, RVCSR::ModifyOp::Write, readOld);
            const word_t expected = supported ? input & ~(asidMask << asidShift) : initial;
            REQUIRE_MSG(result.valid, "SATP write violated MODE rejection, ASIDLEN=0, or old-value semantics");
            REQUIRE_MSG(result.oldValue == (readOld ? initial : 0),
                        "SATP write violated MODE rejection, ASIDLEN=0, or old-value semantics");
            REQUIRE_MSG(result.effect == (supported ? RVCSR::WriteEffect::VmTranslate : RVCSR::WriteEffect::None),
                        "SATP write violated MODE rejection, ASIDLEN=0, or old-value semantics");
            REQUIRE_MSG(read_checked_csr(csr, CSRAddr::SATP) == expected,
                        "SATP write violated MODE rejection, ASIDLEN=0, or old-value semantics");
        }
    }
#ifdef KXEMU_ISA64
    for (auto op : {RVCSR::ModifyOp::Set, RVCSR::ModifyOp::Clear}) {
        write_checked_csr(csr, CSRAddr::SATP, initial);
        const word_t mask = word_t{op == RVCSR::ModifyOp::Set ? 4u : 8u} << modeShift;
        const auto result = csr.modify_csr(CSRAddr::SATP, mask, op);
        REQUIRE_MSG(result.valid, "SATP set/clear accepted unsupported MODE or emitted a write effect");
        REQUIRE_MSG(result.oldValue == initial, "SATP set/clear accepted unsupported MODE or emitted a write effect");
        REQUIRE_MSG(result.effect == RVCSR::WriteEffect::None,
                    "SATP set/clear accepted unsupported MODE or emitted a write effect");
        REQUIRE_MSG(read_checked_csr(csr, CSRAddr::SATP) == initial,
                    "SATP set/clear accepted unsupported MODE or emitted a write effect");
    }
    Machine machine;
    auto core = machine.cpu.get_core(0);
    constexpr word_t pc = entry + 0x100;
    REQUIRE_MSG(core->set_register("satp", initial), "SATP instruction test setup failed");
    core->set_gpr(2, ~word_t{0});
    const unsigned inst = (unsigned(CSRAddr::SATP) << 20) | (2u << 15) | (1u << 12) | (3u << 7) | 0x73u;
    REQUIRE_MSG(machine.bus.write(pc, inst, 4), "SATP instruction test write failed");
    core->set_pc(pc);
    core->step();
    REQUIRE_MSG(core->get_pc() == pc + 4, "unsupported SATP MODE trapped or modified architectural state");
    REQUIRE_MSG(core->get_gpr(3) == initial, "unsupported SATP MODE trapped or modified architectural state");
    REQUIRE_MSG(core->get_register("satp").value() == initial,
                "unsupported SATP MODE trapped or modified architectural state");
#endif
}

TEST_CASE("modify csr operations", "[csr]") {
    using namespace kxemu::cpu;
    RVCSR csr;
    unsigned clockReads = 0;
    csr.init(0, [&] { ++clockReads; return uint64_t{1000000}; });
    csr.reset();
    csr.privMode = PrivMode::MACHINE;
    write_checked_csr(csr, CSRAddr::MSCRATCH, 0x55);
    auto result = csr.modify_csr(CSRAddr::MSCRATCH, 0x0a, RVCSR::ModifyOp::Set);
    REQUIRE_MSG(result.valid, "CSR set returned or stored the wrong value");
    REQUIRE_MSG(result.oldValue == 0x55, "CSR set returned or stored the wrong value");
    REQUIRE_MSG(result.effect == RVCSR::WriteEffect::None, "CSR set returned or stored the wrong value");
    REQUIRE_MSG(read_checked_csr(csr, CSRAddr::MSCRATCH) == 0x5f, "CSR set returned or stored the wrong value");
    result = csr.modify_csr(CSRAddr::MSCRATCH, 0x50, RVCSR::ModifyOp::Clear);
    REQUIRE_MSG(result.valid, "CSR clear returned or stored the wrong value");
    REQUIRE_MSG(result.oldValue == 0x5f, "CSR clear returned or stored the wrong value");
    REQUIRE_MSG(read_checked_csr(csr, CSRAddr::MSCRATCH) == 0xf, "CSR clear returned or stored the wrong value");
    result = csr.modify_csr(CSRAddr::MSCRATCH, 0x1234, RVCSR::ModifyOp::Write);
    REQUIRE_MSG(result.valid, "CSR exchange lost its old value");
    REQUIRE_MSG(result.oldValue == 0xf, "CSR exchange lost its old value");
    REQUIRE_MSG(read_checked_csr(csr, CSRAddr::MSCRATCH) == 0x1234, "CSR exchange lost its old value");
    result = csr.modify_csr(CSRAddr::MSCRATCH, 0x5678, RVCSR::ModifyOp::Write, false);
    REQUIRE_MSG(result.valid, "write-only CSR operation failed");
    REQUIRE_MSG(result.oldValue == 0, "write-only CSR operation failed");
    REQUIRE_MSG(read_checked_csr(csr, CSRAddr::MSCRATCH) == 0x5678, "write-only CSR operation failed");

    for (auto addr : {CSRAddr::SATP, CSRAddr::STIMECMP}) {
        CAPTURE(addr);
        result = csr.modify_csr(addr, 0, RVCSR::ModifyOp::Read);
        REQUIRE_MSG(result.valid, "read-only operation reported write effects");
        REQUIRE_MSG(result.effect == RVCSR::WriteEffect::None, "read-only operation reported write effects");
        result = csr.modify_csr(addr, 0, RVCSR::ModifyOp::Set);
        const auto expected = addr == CSRAddr::SATP ? RVCSR::WriteEffect::VmTranslate : RVCSR::WriteEffect::STimer;
        REQUIRE_MSG(result.valid, "zero-operand write suppressed write effects");
        REQUIRE_MSG(result.effect == expected, "zero-operand write suppressed write effects");
    }
    for (auto op : {RVCSR::ModifyOp::Write, RVCSR::ModifyOp::Set, RVCSR::ModifyOp::Clear}) {
        for (bool readOld : {false, true}) {
            CAPTURE(readOld);
            result = csr.modify_csr(CSRAddr::TIME, 0, op, readOld);
            REQUIRE_MSG(!result.valid, "read-only CSR write invoked time callback or succeeded");
            REQUIRE_MSG(result.effect == RVCSR::WriteEffect::None,
                        "read-only CSR write invoked time callback or succeeded");
            REQUIRE_MSG(clockReads == 0, "read-only CSR write invoked time callback or succeeded");
        }
    }
    result = csr.modify_csr(CSRAddr::TIME, 0, RVCSR::ModifyOp::Read);
    REQUIRE_MSG(result.valid, "TIME read did not query clock exactly once");
    REQUIRE_MSG(clockReads == 1, "TIME read did not query clock exactly once");
    for (auto op : {RVCSR::ModifyOp::Read, RVCSR::ModifyOp::Write, RVCSR::ModifyOp::Set, RVCSR::ModifyOp::Clear}) {
        for (bool readOld : {false, true}) {
            CAPTURE(readOld);
            result = csr.modify_csr(0x7bf, 0, op, readOld);
            REQUIRE_MSG(!result.valid, "unknown CSR operation succeeded");
            REQUIRE_MSG(result.effect == RVCSR::WriteEffect::None, "unknown CSR operation succeeded");
        }
    }
}

TEST_CASE("guest csr operands", "[csr]") {
    using namespace kxemu::cpu;
    for (unsigned funct : {1u, 2u, 3u, 5u, 6u, 7u}) {
        for (word_t operand : {word_t{0}, word_t{0xa}}) {
            CAPTURE(funct, operand);
            Machine machine;
            auto core = machine.cpu.get_core(0);
            constexpr word_t pc = entry + 0x100;
            core->set_register("mscratch", 0x55);
            core->set_gpr(2, operand);
            const unsigned source = funct >= 5 ? unsigned(operand) : 2u;
            // rd=rs1 for register forms: writeback must not overwrite the operand early.
            const unsigned inst = (unsigned(CSRAddr::MSCRATCH) << 20) | (source << 15) |
                                  (funct << 12) | (2u << 7) | 0x73u;
            REQUIRE_MSG(machine.bus.write(pc, inst, 4), "CSR operand test instruction write failed");
            core->set_pc(pc);
            core->step();
            const word_t expected = funct == 1 || funct == 5 ? operand
                : funct == 2 || funct == 6 ? 0x55 | operand : 0x55 & ~operand;
            REQUIRE_MSG(core->get_gpr(2) == 0x55, "CSR instruction lost operand or returned wrong old value");
            REQUIRE_MSG(core->get_register("mscratch").value() == expected,
                        "CSR instruction lost operand or returned wrong old value");
        }
        for (unsigned source : {0u, 2u}) {
            CAPTURE(funct, source);
            Machine machine;
            auto core = machine.cpu.get_core(0);
            constexpr word_t pc = entry + 0x100;
            constexpr word_t sentinel = 0x12345678;
            core->set_gpr(2, 0); // A non-x0 register containing zero still requests a write.
            core->set_gpr(3, sentinel);
            const unsigned inst = (unsigned(CSRAddr::MVENDORID) << 20) | (source << 15) |
                                  (funct << 12) | (3u << 7) | 0x73u;
            REQUIRE_MSG(machine.bus.write(pc, inst, 4), "read-only CSR test instruction write failed");
            core->set_pc(pc);
            core->step();
            const bool writes = funct == 1 || funct == 5 || source != 0;
            if (writes) {
                REQUIRE_MSG(core->get_register("mcause").value() == TrapCode::ILLEGAL_INST,
                            "rejected CSR write changed destination or did not trap");
                REQUIRE_MSG(core->get_gpr(3) == sentinel, "rejected CSR write changed destination or did not trap");
            } else {
                REQUIRE_MSG(core->get_pc() == pc + 4, "zero-source CSR read attempted a write");
                REQUIRE_MSG(core->get_gpr(3) == KXEMU_VENDORID, "zero-source CSR read attempted a write");
            }
        }
    }
}

