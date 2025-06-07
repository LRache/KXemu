#ifndef __KXEMU_CPU_RISCV_CORE_H__
#define __KXEMU_CPU_RISCV_CORE_H__

#include "cpu/core.h"
#include "cpu/riscv/aclint.h"
#include "cpu/riscv/addr.h"
#include "cpu/riscv/def.h"
#include "cpu/riscv/plic.h"
#include "cpu/riscv/pte.h"
#include "cpu/word.h"
#include "cpu/riscv/csr.h"
#include "device/bus.h"

#include <expected>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace kxemu::cpu {

class RVCore : public Core<word_t>{
private:
    unsigned int coreID;

    bool debugMode = false;

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
        DontCare = 0,
        LOAD  = 1 << 1,
        STORE = 1 << 2,
        FETCH = 1 << 3,
        AMO   = LOAD | STORE,
    };
    device::Bus *bus;
    device::AClint *aclint;
    device::PLIC *plic;
    std::mutex *deviceMtx;
    void update_device();
    bool memory_fetch();
    std::optional<word_t> memory_load(word_t addr, unsigned int len);
    void memory_store(word_t addr, word_t data, unsigned int len);

    // Virtual address translation
    std::optional<word_t> pm_read(word_t paddr, unsigned int len);
    bool pm_write(word_t paddr, word_t data, unsigned int len);
    std::optional<word_t> pm_read_check (word_t paddr, unsigned int len); // With PMP check
    bool pm_write_check(word_t paddr, word_t data, unsigned int len);
    bool vm_fetch();
    std::optional<word_t> vm_read(word_t vaddr, unsigned int len);
    bool vm_write(word_t vaddr, word_t  data, unsigned int len);

    enum class VMFault {
        PAGE_FAULT,
        ACCESS_FAULT,
    };
    using VMResult = std::expected<word_t, VMFault>;
    VMResult vaddr_translate_core(addr_t addr, MemType type);
    
    template<unsigned int LEVELS, unsigned int PTESIZE, unsigned int VPNBITS>
    VMResult vaddr_translate_sv(addr_t vaddr, MemType type); // The template function for sv32, sv39, sv48, sv57
    
    VMResult vaddr_translate_bare(word_t vaddr, MemType type);
    #ifdef KXEMU_ISA32
    VMResult vaddr_translate_sv32(word_t addr, MemType type);
    #else
    VMResult vaddr_translate_sv39(word_t vaddr, MemType type);
    VMResult vaddr_translate_sv48(word_t vaddr, MemType type);
    VMResult vaddr_translate_sv57(word_t vaddr, MemType type);
    #endif
    
    word_t pageTableBase;
    VMResult (RVCore::*vaddr_translate_func)(word_t addr, MemType type);
    // word_t (RVCore::*vaddr_translate_func)(word_t addr, MemType type, VMResult &result);
    void update_vm_translate();

    // Physical memory protection
    bool pmp_check_x(word_t paddr, unsigned int len);
    bool pmp_check_r(word_t paddr, unsigned int len);
    bool pmp_check_w(word_t paddr, unsigned int len);

    // Decoder
    struct DecodeInfo {
        uint8_t rd;
        uint8_t rs1;
        union {
            struct {
                uint8_t rs2;
                uint8_t flag;
            };
            uint16_t csr;
        };
        union {
            word_t imm;
            word_t npc;
            uint8_t rs3;
        };

    #ifdef CONFIG_DEBUG_DECODER
        bool rd_set;
        bool rs1_set;
        bool rs2_set;
        bool rs3_set;
        bool flag_set;
        bool csr_set;
        bool imm_set;
        bool npc_set;
    #endif
    };
    DecodeInfo gDecodeInfo;
    
    typedef void (RVCore::*do_inst_t)(const DecodeInfo &decodeInfo);
    do_inst_t decode_and_exec  (DecodeInfo &decodeInfo);
    do_inst_t decode_and_exec_c(DecodeInfo &decodeInfo); // for compressed instructions

    #include "./local-include/decode-list.h"

    // running
    uint32_t inst;
    word_t pc;
    word_t npc;
    void execute();
    void run_step(unsigned int &counter);

    // Trap
    void trap(TrapCode code, word_t value = 0);
    
    // Interrupt
    void   set_interrupt(InterruptCode code);
    void clear_interrupt(InterruptCode code);
    bool scan_interrupt();
    void interrupt_m(InterruptCode code);
    void interrupt_s(InterruptCode code);

    // Do instructions
    void do_invalid_inst();
    void do_invalid_inst(const DecodeInfo &decodeInfo);
    #include "./local-include/inst-list.h"

    // Zicsr and Privilleged
    RVCSR csr;
    unsigned int &privMode = csr.privMode; // Priviledge mode
    void init_csr();
    word_t  read_csr(unsigned int addr, bool &valid);
    bool   write_csr(unsigned int addr, word_t value);
    
    void update_mstatus();
    struct {
        bool mie;
        bool sie;
        bool sum;
    } mstatus;

    const word_t *mie;
    const word_t *mip;
    const word_t *medeleg;
    const word_t *medelegh;
    const word_t *mideleg;

    std::mutex csrMtx;

    void set_priv_mode(int mode);

    uint64_t get_uptime();
    void update_stimecmp();

    // Atomic extension
    std::unordered_map<word_t, word_t> reservedMemory; // for lr, sc
    std::optional<word_t> amo_vaddr_translate_and_set_trap(word_t vaddr, int len);
    template<typename sunit_t> void do_load_reserved(const DecodeInfo &decodeInfo);
    template<typename sunit_t> void do_store_conditional(const DecodeInfo &decodeInfo);
    template<device::AMO amo, typename sw_t> void do_amo_inst(const DecodeInfo &decodeInfo);

    word_t gpr[33];
    
    // Floating-point extension
    static_assert(sizeof(float) == 4, "sizeof(float) != 4");
    static_assert(sizeof(double) == 8, "sizeof(double) != 8");
    union {
        double f64;
        struct {
            uint32_t high;
            float f32;
        };
    } fpr[32];
    unsigned int frm;
    void update_fcsr();

    // Experimental ICache
    #ifdef CONFIG_ICache
    struct ICacheBlock {
        word_t tag;
        
        do_inst_t do_inst;
        DecodeInfo decodeInfo;
        unsigned int instLen;
        bool valid = false;
    };
    ICacheBlock icache[1 << ICACHE_SET_BITS];
    #endif
    void icache_push(do_inst_t do_inst, unsigned int instLen, const DecodeInfo &decodeInfo);
    bool icache_decode_and_exec();
    void icache_fence();

    static constexpr unsigned int TLB_SET_BITS = 5;
    struct TLBBlock {
        word_t paddr;
        word_t tag;
        word_t pteAddr;
        bool valid = false;
        PTEFlag flag;
    };
    TLBBlock tlb[1 << TLB_SET_BITS];
    void tlb_push(addr_t vaddr, addr_t paddr, word_t pteAddr, uint8_t type);
    std::optional<TLBBlock *> tlb_hit(addr_t vaddr);
    void tlb_fence();

public:
    RVCore();
    ~RVCore();

    void init(unsigned int coreID, device::Bus *bus, device::AClint *aclint, device::PLIC *plic);
    
    void reset(word_t entry) override;
    void step() override;
    void run(const word_t *breakpoints = nullptr, unsigned int n = 0) override;
    void set_device_mtx(std::mutex *mtx);

    void set_debug_mode(bool debug) {
        this->debugMode = debug;
    }
    
    bool is_error()   override;
    bool is_break()   override;
    bool is_running() override;
    bool is_halt()    override;

    word_t get_pc() override;
    void   set_pc(word_t pc) override;
    word_t get_gpr(unsigned int idx) override;
    void   set_gpr(unsigned int idx, word_t value) override;
    std::optional<word_t> get_register(const std::string &name) override;
    bool   set_register(const std::string &name, word_t value) override;

    word_t get_halt_pc() override;
    word_t get_halt_code() override;

    word_t vaddr_translate(word_t vaddr, bool &valid) override;
    
    void   set_timer_interrupt_m();
    void   set_timer_interrupt_s();
    void clear_timer_interrupt_m();
    void clear_timer_interrupt_s();

    void   set_software_interrupt_m();
    void   set_software_interrupt_s();
    void clear_software_interrupt_m();
    void clear_software_interrupt_s();
    
    void   set_external_interrupt_m();
    void   set_external_interrupt_s();
    void clear_external_interrupt_m();
    void clear_external_interrupt_s();
};

} // namespace kxemu::cpu

#endif
