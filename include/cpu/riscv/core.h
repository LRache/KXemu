#ifndef __KXEMU_CPU_RISCV_CORE_H__
#define __KXEMU_CPU_RISCV_CORE_H__

#include "cpu/core.h"
#include "cpu/riscv/aclint.h"
#include "cpu/word.h"
#include "cpu/riscv/csr.h"
#include "device/bus.h"
#include "utils/decoder.h"
#include "utils/task-timer.h"

#include <cstdint>
#include <unordered_set>

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
    };
    device::Bus *bus;
    AClint *aclint;
    bool fetch_inst();
    word_t memory_load (word_t addr, int len);
    bool   memory_store(word_t addr, word_t data, int len);

    // Virtual address translation
    enum VMResult {
        VM_OK,
        VM_PAGE_FAULT,
        VM_ACCESS_FAULT,
    };
    word_t vaddr_translate(word_t addr, MemType type, VMResult &result);
    word_t vaddr_translate_sv(word_t addr, MemType type, VMResult &result, int levels, word_t pteSize, unsigned int vpnBits); // The template function for sv32, sv39, sv48, sv57
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
    // utils::Decoder<RVCore>  decoder32;
    // utils::Decoder<RVCore> cdecoder32;
    // utils::Decoder<RVCore>  decoder64;
    // utils::Decoder<RVCore> cdecoder64; 
    // utils::Decoder<RVCore>  *decoder;
    // utils::Decoder<RVCore> *cdecoder; // for compressed instructions
    utils::Decoder<RVCore>  decoder;
    utils::Decoder<RVCore> cdecoder; // for compressed instructions
    void build_decoder();

    // running
    uint32_t inst;
    word_t pc;
    word_t npc;
    void execute();
    std::unordered_set<word_t> breakpoints;

    // Trap
    bool trapFlag;
    void trap(word_t code, word_t value = 0);
    
    // Interrupt
    bool check_timer_interrupt();
    bool check_external_interrupt();

    void set_interrupt(word_t code);
    void clear_interrupt(word_t code);
    bool scan_interrupt();
    void interrupt_m(word_t code);
    void interrupt_s(word_t code);
    
    word_t gpr[32];
    void set_gpr(int index, word_t value);

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

    // Timer interrupt
    uint64_t bootTime;
    uint64_t get_uptime();
    utils::TaskTimer *taskTimer = nullptr;

    uint64_t mtime;
    uint64_t mtimecmp;
    unsigned int mtimerTaskID;
    unsigned int stimerTaskID;
    void update_mtimecmp();
    void update_stimecmp();

public:
    RVCore();
    ~RVCore();

    void init(unsigned int coreID, device::Bus *bus, int flags, AClint *aclint, utils::TaskTimer *timer);
    
    void reset(word_t entry) override;
    void step() override;
    void run() override;
    void run(word_t *breakpoints, unsigned int n) override;
    
    bool is_error()   override;
    bool is_break()   override;
    bool is_running() override;
    bool is_halt()    override;

    word_t get_pc() override;
    word_t get_gpr(int idx) override;
    word_t get_register(const std::string &name, bool &success) override;

    word_t get_halt_pc() override;
    word_t get_halt_code() override;

    word_t vaddr_translate(word_t vaddr, bool &valid) override;
    
    void set_external_interrupt_m();
    void set_external_interrupt_s();
    void clear_external_interrupt_m();
    void clear_external_interrupt_s();
    void increase_uptime();
};

} // namespace kxemu::cpu

#endif
