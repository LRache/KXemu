#ifndef __ISA_RISCV32_CORE_H__
#define __ISA_RISCV32_CORE_H__

#include "cpu/cpu.h"
#include "cpu/decoder.h"
#include "isa/riscv32/csr.h"
#include "isa/riscv32/isa.h"
#include "isa/word.h"
#include "memory/memory.h"

#include <chrono>
#include <ratio>

class RV32Core : public Core {
private:
    int flags;
    using word_t = uint32_t;
    using sword_t = int32_t;

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
    Memory *memory;
    bool fetch_inst();
    word_t memory_load(word_t addr, int len);
    bool memory_store(word_t addr, word_t data, int len);
    bool check_pmp(word_t addr, int len, int type);

    // decoder
    Decoder<RV32Core> decoder;
    Decoder<RV32Core> cdecoder; // for compressed instructions

    // running
    uint32_t inst;
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

    void do_mul();
    void do_mulh();
    void do_mulhsu();
    void do_mulhu();
    void do_div();
    void do_divu();
    void do_rem();
    void do_remu();
    
    void do_invalid_inst();

    // Compressed extension
    void do_c_lwsp();
    void do_c_swsp();
    
    void do_c_lw();
    void do_c_sw();

    void do_c_j();
    void do_c_jal();
    void do_c_jr();
    void do_c_jalr();

    void do_c_beqz();
    void do_c_bnez();

    void do_c_li();
    void do_c_lui();

    void do_c_addi();
    void do_c_addi16sp();
    void do_c_addi4spn();

    void do_c_slli();
    void do_c_srli();
    void do_c_srai();
    void do_c_andi();

    void do_c_mv();
    void do_c_add();
    void do_c_sub();
    void do_c_xor();
    void do_c_or();
    void do_c_and();

    // Privileged mode
    enum PrivMode {
        MACHINE = 3,
        SUPERVISOR = 1,
        USER = 0,
    };
    int privMode;
    void do_ecall();
    void do_mret();
    void do_sret();
    void do_ebreak();

    // Zicsr exntension
    void do_csrrw();
    void do_csrrs();
    void do_csrrc();
    void do_csrrwi();
    void do_csrrsi();
    void do_csrrci();

    RV32CSR csr;
    void init_csr();
    word_t get_csr(unsigned int addr, bool &success);
    void   set_csr(unsigned int addr, word_t value, bool &success);

    // CSR reference
    word_t *mcause;
    word_t *mepc;
    const word_t *mtvec;
    word_t *mstatus;
    word_t *mtval;
    const word_t *mie;
    word_t *mip;
    const word_t *medeleg;
    const word_t *medelegh;
    const word_t *mideleg;

    word_t *scause;
    word_t *sepc;
    const word_t *stvec;
    word_t *stval;
    const word_t *sie;

    // timer interrupt
    uint64_t mtime;
    uint64_t mtimecmp;
    std::chrono::duration<uint64_t, std::nano> uptime;
    std::chrono::duration<uint64_t, std::nano> uptimecmp;
    bool timerIntrruptNotTriggered;

public:
    void init(Memory *memory, int flags);
    void reset(word_t entry = INIT_PC);
    void step() override;
    bool is_error() override;
    bool is_break() override;
    bool is_running() override;

    word_t get_pc() override;
    word_t get_gpr(int idx) override;

    word_t get_halt_pc() override;
    word_t get_halt_code() override;

    ~RV32Core();
};

#endif
