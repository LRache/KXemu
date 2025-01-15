#ifndef __KXEMU_CPU_RISCV32_CORE_H__
#define __KXEMU_CPU_RISCV32_CORE_H__

#include "cpu/cpu.h"
#include "utils/decoder.h"
#include "cpu/riscv32/csr.h"
#include "device/bus.h"
#include <cstdint>

namespace kxemu::cpu {

class RV32Core : public Core<uint32_t> {
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
    device::Bus *bus;
    bool fetch_inst();
    word_t memory_load(word_t addr, int len);
    bool memory_store(word_t addr, word_t data, int len);
    bool check_pmp(word_t addr, int len, int type);

    // decoder
    utils::Decoder<RV32Core> decoder;
    utils::Decoder<RV32Core> cdecoder; // for compressed instructions

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

    #include "./local-include/inst.h"

    // priviledge mode
    enum PrivMode {
        MACHINE = 3,
        SUPERVISOR = 1,
        USER = 0,
    };
    int privMode;

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
    uint64_t uptime;
    uint64_t uptimecmp;
    bool timerIntrruptNotTriggered;

public:
    void init(device::Bus *bus, int flags);
    void reset(word_t entry);
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

} // namespace kxemu::cpu

#endif
