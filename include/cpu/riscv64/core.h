#ifndef __KXEMU_ISA_RISCV64_CORE_H__
#define __KXEMU_ISA_RISCV64_CORE_H__

#include "cpu/cpu.h"
#include "cpu/decoder.h"

class RV64Core : public Core {
private:
    int flags;
    using word_t = uint64_t;
    using sword_t = int64_t;

    enum state_t {
        IDLE,
        RUNNING,
        ERROR,
        BREAK,
    } state;

    word_t haltCode;
    word_t haltPC;

    enum MemType {
        FETCH,
        LOAD,
        STROE,
    };
    Bus *bus;
    bool fetch_inst();
    word_t memory_load(word_t addr, int len);
    bool memory_store(word_t addr, word_t data, int len);
    bool check_pmp(word_t addr, int len, int type);

    // decoder
    Decoder<RV64Core> decoder;
    Decoder<RV64Core> cdecoder; // for compressed instructions

    // running
    uint64_t inst;
    word_t pc;
    word_t npc;
    void execute();

    // Trap and Interrupt
    bool trapFlag;
    void trap(word_t code, word_t value = 0);
    void interrupt(word_t code);
    bool scan_interrupt();
    void interrupt_m(word_t code);
    void interrupt_s(word_t code);
    
    word_t gpr[32];
    void set_gpr(int index, word_t value);

    // do instructions
    void init_decoder();

public:
    void init(Bus *memory, int flags);
    void reset(word_t pc);
    
    word_t get_halt_pc() override;
    word_t get_pc() override;
    bool is_error() override;
    bool is_break() override;
    bool is_running() override;

    void step() override;
};

#endif
