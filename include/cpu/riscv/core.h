#ifndef __KXEMU_CPU_RISCV_CORE_H__
#define __KXEMU_CPU_RISCV_CORE_H__

#include "cpu/core.h"
#include "cpu/riscv/aclint.h"
#include "cpu/word.h"
#include "cpu/riscv/csr.h"
#include "device/bus.h"
#include "utils/task-timer.h"
#include "utils/decoder.h"

#include <unordered_map>
#include <cstdint>

namespace kxemu::cpu {

class RVCore : public Core<word_t>{
private:
    unsigned int coreID;
    int flags;

    enum state_t {
        IDLE,
        RUNNING,
        ERROR,
        BREAKPOINT,
        HALT,
    } state;

    word_t haltCode;
    word_t haltPC;

    // Memory access
    enum MemType {
        STORE = 1 << 1,
        LOAD  = 1 << 2,
        FETCH = 1 << 3,
        AMO   = 1 << 6,
    };
    device::Bus *bus;
    device::AClint *aclint;
    bool   memory_fetch();
    word_t memory_load (word_t addr, int len);
    bool   memory_store(word_t addr, word_t data, int len);

    // Virtual address translation
    enum VMResult {
        VM_OK,
        VM_PAGE_FAULT,
        VM_ACCESS_FAULT,
    };
    word_t vaddr_translate(word_t addr, MemType type, VMResult &result);
    template<unsigned int LEVELS, unsigned int PTESIZE, unsigned int VPNBITS>
    word_t vaddr_translate_sv(word_t addr, MemType type, VMResult &result); // The template function for sv32, sv39, sv48, sv57
    #ifdef KXEMU_ISA32
    word_t vaddr_translate_sv32(word_t addr, MemType type, VMResult &result);
    #else
    word_t vaddr_translate_sv39(word_t addr, MemType type, VMResult &result);
    word_t vaddr_translate_sv48(word_t addr, MemType type, VMResult &result);
    word_t vaddr_translate_sv57(word_t addr, MemType type, VMResult &result);
    #endif

    // Physical memory protection
    bool check_pmp(word_t addr, int len, MemType type);

    // decoder
    utils::Decoder<RVCore>  decoder; // UNUSED
    utils::Decoder<RVCore> cdecoder; // UNUSED
    void build_decoder(); // UNUSED
    bool decode_and_exec();
    bool decode_and_exec_c(); // for compressed instructions

    // running
    uint32_t inst;
    word_t pc;
    word_t npc;
    void execute();

    // Trap
    bool trapFlag;
    void trap(word_t code, word_t value = 0);
    
    // Interrupt
    void set_interrupt(word_t code);
    void clear_interrupt(word_t code);
    bool scan_interrupt();
    void interrupt_m(word_t code);
    void interrupt_s(word_t code);
    
    word_t gpr[32];

    // do instructions
    void do_invalid_inst();
    #include "./local-include/inst-list.h"

    RVCSR csr;
    int &privMode = csr.privMode; // Priviledge mode
    void init_csr();
    word_t  read_csr(unsigned int addr, bool &valid);
    bool   write_csr(unsigned int addr, word_t value);

    bool msip; // Machine-mode Software Interrupt Pending Flag
    bool ssip; // Supervisor-mode Software Interrupt Pending Flag

    word_t *mstatus;
    const word_t *mie;
    const word_t *mip;
    const word_t *medeleg;
    const word_t *medelegh;
    const word_t *mideleg;

    const word_t *satp;

    uint64_t get_uptime();
    void update_stimecmp();

    // Atomic extension
    std::unordered_map<word_t, word_t> reservedMemory; // for lr, sc
    word_t amo_vaddr_translate_and_set_trap(word_t vaddr, int len, bool &valid);
    template<int len> void do_load_reserved();
    template<int len> void do_store_conditional();
    template<device::AMO amo, typename sw_t = int32_t> void do_amo_inst();

public:
    RVCore();
    ~RVCore();

    void init(unsigned int coreID, device::Bus *bus, int flags, device::AClint *aclint, utils::TaskTimer *timer);
    
    void reset(word_t entry) override;
    void step() override;
    void run(const word_t *breakpoints = nullptr, unsigned int n = 0) override;
    
    bool is_error()   override;
    bool is_break()   override;
    bool is_running() override;
    bool is_halt()    override;

    word_t get_pc() override;
    void   set_pc(word_t pc) override;
    word_t get_gpr(unsigned int idx) override;
    void   set_gpr(unsigned int idx, word_t value) override;
    word_t get_register(const std::string &name, bool &success) override;

    word_t get_halt_pc() override;
    word_t get_halt_code() override;

    word_t vaddr_translate(word_t vaddr, bool &valid) override;
    
    void set_timer_interrupt_m();
    void set_timer_interrupt_s();
    void clear_timer_interrupt_m();
    void clear_timer_interrupt_s();

    void set_software_interrupt_m();
    void set_software_interrupt_s();
    void clear_software_interrupt_m();
    void clear_software_interrupt_s();
    
    void set_external_interrupt_m();
    void set_external_interrupt_s();
    void clear_external_interrupt_m();
    void clear_external_interrupt_s();
};

} // namespace kxemu::cpu

#endif
