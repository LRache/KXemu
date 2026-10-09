#include "catch-support.hpp"
#include "cpu/riscv/cpu.hpp"
#include "cpu/riscv/csr-field.hpp"
#include "cpu/riscv/def.hpp"

#include <catch2/generators/catch_generators.hpp>

#ifndef KXEMU_ISA64
#error "Sv48/Sv57 superpage tests require RV64"
#endif

using kxemu::cpu::RVCPU;
using kxemu::cpu::word_t;
using kxemu::device::Bus;
namespace csr = kxemu::cpu::csr;

constexpr word_t entry    = 0x80000000;
constexpr word_t ramSize  = 0x8000;
constexpr word_t sCode    = entry + 0x100; // S-mode code, reached by MRET.
constexpr word_t handler  = entry + 0x200; // M-mode trap handler.
constexpr word_t dataPA   = entry + 0x2000;
constexpr word_t rootPA   = entry + 0x4000;
constexpr word_t pageBits = 12;
constexpr word_t vpnBits  = 9;

constexpr word_t PTE_V = 1 << 0, PTE_R = 1 << 1, PTE_W = 1 << 2, PTE_X = 1 << 3;
constexpr word_t PTE_A = 1 << 6, PTE_D = 1 << 7;
constexpr word_t leafFlags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;

constexpr word_t INST_MRET  = 0x30200073;
constexpr word_t INST_LD    = 0x00033283; // ld x5, 0(x6)
constexpr word_t INST_SD    = 0x00733423; // sd x7, 8(x6)

struct SuperpageCase {
    const char *name;
    csr::Satp::Mode mode;
    unsigned int levels;
};

// The root-level leaf covers 2^(12 + 9 * (levels - 1)) bytes: 512 GiB for Sv48, 256 TiB for Sv57.
static word_t root_va(const SuperpageCase &c, word_t index, word_t offset) {
    return (index << (pageBits + vpnBits * (c.levels - 1))) | offset;
}

struct SuperpageMachine {
    Bus bus;
    RVCPU cpu;
    kxemu::cpu::RVCore *core;

    explicit SuperpageMachine(const SuperpageCase &c) {
        REQUIRE_MSG(bus.add_memory_map(entry, ramSize), "RAM map failed");
        cpu.init(&bus, 0, 1);
        cpu.reset(entry);
        core = cpu.get_core(0);

        REQUIRE_MSG(bus.write(entry, INST_MRET, 4), "MRET write failed");
        REQUIRE_MSG(bus.write(sCode, INST_LD, 4), "LD write failed");
        REQUIRE_MSG(bus.write(sCode + 4, INST_SD, 4), "SD write failed");
        REQUIRE_MSG(bus.write(handler, 0x00000013, 4), "handler write failed");

        // root[0]: identity map so S-mode can fetch code at its physical address.
        // root[1]: maps root_va(1, x) to physical x (PPN 0, aligned superpage).
        // root[2]: misaligned superpage, PPN bit 20 lies inside the low PPN fields.
        REQUIRE_MSG(bus.write(rootPA + 0 * 8, leafFlags, 8), "PTE write failed");
        REQUIRE_MSG(bus.write(rootPA + 1 * 8, leafFlags, 8), "PTE write failed");
        REQUIRE_MSG(bus.write(rootPA + 2 * 8, (word_t{1} << 20) << 10 | leafFlags, 8), "PTE write failed");

        csr::Satp satp(0);
        satp.set_mode(c.mode);
        satp = word_t(satp) | (rootPA >> pageBits);
        REQUIRE_MSG(core->set_register("satp", satp), "satp write failed");
        REQUIRE_MSG(core->set_register("mtvec", handler), "mtvec write failed");
        REQUIRE_MSG(core->set_register("mepc", sCode), "mepc write failed");
        REQUIRE_MSG(core->set_register("mstatus", word_t{1} << 11), "mstatus write failed"); // MPP=S
      
              core->set_pc(entry);
        core->step(); // MRET into S-mode
        REQUIRE_MSG(core->get_pc() == sCode, "MRET did not enter S-mode code");
        REQUIRE_MSG(core->get_register("priv").value() == kxemu::cpu::PrivMode::SUPERVISOR,
                    "MRET did not switch to S-mode");
    }
};

static const SuperpageCase superpageCases[] = {
    {"Sv48", csr::Satp::SV48, 4},
    {"Sv57", csr::Satp::SV57, 5},
};

TEST_CASE("aligned root-level superpage", "[mmu][superpage]") {
    const auto &c = superpageCases[GENERATE(0, 1)];
    INFO("mode = " << c.name);
    SuperpageMachine m(c);
    constexpr word_t value = 0x1122334455667788;
    constexpr word_t stored = 0x0123456789abcdef;
    REQUIRE_MSG(m.bus.write(dataPA, value, 8), "data write failed");

    m.core->set_gpr(6, root_va(c, 1, dataPA));
    m.core->set_gpr(7, stored);
    m.core->step(); // ld
    INFO("mcause = " << m.core->get_register("mcause").value());
    REQUIRE_MSG(m.core->get_pc() == sCode + 4, "root-level superpage load trapped");
    REQUIRE_MSG(m.core->get_gpr(5) == value, "root-level superpage load read wrong data");

    m.core->step(); // sd
    REQUIRE_MSG(m.core->get_pc() == sCode + 8, "root-level superpage store trapped");
    bool valid = false;
    const word_t written = m.bus.read(dataPA + 8, 8, valid);
    REQUIRE_MSG(valid, "root-level superpage store target is unreadable");
    REQUIRE_MSG(written == stored, "root-level superpage store wrote wrong physical address");
}

TEST_CASE("misaligned root-level superpage", "[mmu][superpage]") {
    const auto &c = superpageCases[GENERATE(0, 1)];
    INFO("mode = " << c.name);
    SuperpageMachine m(c);
    m.core->set_gpr(6, root_va(c, 2, dataPA));
    m.core->step(); // ld
    REQUIRE_MSG(m.core->get_pc() == handler, "misaligned root-level superpage did not trap");
    REQUIRE_MSG(m.core->get_register("mcause").value() == kxemu::cpu::TrapCode::LOAD_PAGE_FAULT,
                "misaligned root-level superpage did not raise load page fault");
}

