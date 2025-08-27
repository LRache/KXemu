#ifndef __KXEMU_CPU_LOONGARCH_CORE_H__
#define __KXEMU_CPU_LOONGARCH_CORE_H__

#include "cpu/core.h"
#include "cpu/word.h"
#include "device/bus.h"
#include <cstdint>

namespace kxemu::cpu {

class LACore : public Core<word_t> {
private:
    enum state_t {
        IDLE,
        RUNNING,
        ERROR,
        BREAKPOINT,
        HALT,
    } state;
    word_t haltPC;
    word_t haltCode;

    word_t gpr[32];

    // Memory
    device::Bus *bus;
    bool   memory_fetch();
    bool   memory_store(word_t addr, word_t data, int len);
    word_t memory_load (word_t addr, int len, bool &success);

    // Excute
    word_t pc;
    word_t npc;
    uint32_t inst;
    void execute();
    bool decode_and_exec();

    // do instruction
    void do_illegal_inst();
    #include "./local-include/inst-list.h"

public:
    LACore();
    ~LACore() override = default;

    bool is_error() override;
    bool is_break() override;
    bool is_running() override;
    bool is_halt() override;

    void reset(word_t entry) override;
    void step() override;
    void run(const word_t *breakpoints = nullptr, unsigned int n = 0) override;

    word_t get_pc() override;
    void   set_pc(word_t pc) override;
    word_t get_gpr(unsigned int index) override;
    void   set_gpr(unsigned int index, word_t value) override;
    word_t get_register(const std::string &name, bool &success) override;
    
    word_t get_halt_pc() override;
    word_t get_halt_code() override;

    void init(unsigned int coreID, device::Bus *bus, int flags);
};

}

#endif
