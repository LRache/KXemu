#ifndef __ISA_RISCV32_CORE_H__
#define __ISA_RISCV32_CORE_H__

#include "cpu/cpu.h"
#include "cpu/decoder.h"
#include "memory/memory.h"

#include <cstdint>

class RV32Core : public Core {
private:
    Memory *memory;
    int flags;
    enum state_t {
        IDLE,
        RUNNING,
        ERROR,
        BREAK,
    } state;
    Decoder<RV32Core> decoder;

    using word_t = uint32_t;
    word_t pc;
    word_t gpr[32];
    void set_gpr(int index, word_t value);

    uint32_t inst;
    word_t npc;
    void execute();

    void do_add();
    
    void do_addi();
    
    void do_ebreak();
    void do_invalid_inst();

    void init_decoder();

public:
    void init(Memory *memory, int flags);
    void reset();
    void step();
    bool is_error() override;
    bool is_break() override;

    ~RV32Core();
};

#endif
