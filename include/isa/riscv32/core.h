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
    using sword_t = int32_t;
    word_t pc;
    word_t gpr[32];
    void set_gpr(int index, word_t value);

    word_t trapCode;
    word_t trapPC;
    word_t haltPC;

    uint32_t inst;
    word_t npc;
    void execute();

    word_t mem_read(word_t addr, int len);
    int mem_write(word_t addr, word_t data, int len);

    // do instructions
    void do_add();
    void do_sub();
    void do_and();
    void do_or();
    void do_xor();
    void do_sll();
    void do_srl();
    void do_sra();
    void do_slt();
    void do_sltu();
    
    void do_addi();
    void do_andi();
    void do_ori();
    void do_xori();
    void do_slli();
    void do_srli();
    void do_srai();
    void do_slti();
    void do_sltiu();

    void do_lb();
    void do_lbu();
    void do_lh();
    void do_lhu();
    void do_lw();

    void do_sb();
    void do_sh();
    void do_sw();

    void do_beq();
    void do_bge();
    void do_bgeu();
    void do_blt();
    void do_bltu();
    void do_bne();

    void do_jal();
    void do_jalr();

    void do_lui();
    void do_auipc();
    
    void do_ebreak();
    void do_invalid_inst();

    void init_decoder();

public:
    void init(Memory *memory, int flags);
    void reset();
    void step() override;
    bool is_error() override;
    bool is_break() override;

    word_t get_pc() override;
    word_t get_gpr(int idx) override;

    word_t get_trap_pc() override;
    word_t get_trap_code() override;
    word_t get_halt_pc() override;

    ~RV32Core();
};

#endif
