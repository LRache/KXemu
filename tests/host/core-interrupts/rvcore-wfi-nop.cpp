#include "catch-support.hpp"
#include "cpu/riscv/cpu.hpp"

#include <catch2/generators/catch_generators.hpp>

#ifndef CONFIG_WFI_AS_NOP
#error "This test requires CONFIG_WFI_AS_NOP"
#endif

using kxemu::cpu::RVCPU;
using kxemu::cpu::word_t;
using kxemu::device::Bus;

constexpr word_t entry = 0x80000000;
constexpr word_t software = word_t{1} << 3;

struct Machine {
    Bus bus;
    RVCPU cpu;
    kxemu::cpu::RVCore *core;

    Machine() {
        REQUIRE_MSG(bus.add_memory_map(entry, 4096), "RAM map failed");
        REQUIRE_MSG(bus.write(entry, 0x10500073, 4), "WFI write failed");
        REQUIRE_MSG(bus.write(entry + 4, 0x00000013, 4), "NOP write failed");
        cpu.init(&bus, 0, 1);
        core = cpu.get_core(0);
    }
};

TEST_CASE("WFI executes as NOP", "[wfi]") {
    Machine machine;
    auto core = machine.core;
    // WFI must complete with no pending IRQ, a masked IRQ, or a locally enabled
    // IRQ. Global interrupt delivery stays disabled by reset in all cases.
    const unsigned int scenario = GENERATE(0u, 1u, 2u);
    CAPTURE(scenario);
    machine.cpu.reset(entry);
    core->set_gpr(10, 42);
    if (scenario != 0) core->set_software_interrupt_m();
    if (scenario == 2) core->set_register("mie", software);
    core->step();
    REQUIRE_MSG(core->get_pc() == entry + 4, "WFI did not advance as NOP");
    REQUIRE_MSG(core->get_gpr(10) == 42, "WFI changed a GPR");
    REQUIRE_MSG(core->is_running(), "WFI stopped the core");
    const word_t expectedPending = scenario == 0 ? 0 : software;
    REQUIRE_MSG((core->get_register("mip").value() & software) == expectedPending,
                "normal interrupt synchronization changed");
    core->step();
    REQUIRE_MSG(core->get_pc() == entry + 8, "instruction after WFI did not execute");
}

TEST_CASE("run passes WFI without an interrupt producer", "[wfi][stop]") {
    Machine machine;
    auto core = machine.core;
    machine.cpu.reset(entry);
    const word_t breakpoint = entry + 4;
    core->run(&breakpoint, 1);
    REQUIRE_MSG(core->get_pc() == breakpoint, "run did not pass WFI");
    core->request_stop();
    core->step();
    REQUIRE_MSG(core->get_pc() == breakpoint, "stop request was ignored");
    machine.cpu.reset(entry);
    core->step();
    REQUIRE_MSG(core->get_pc() == entry + 4, "reset did not clear stop request");
}
