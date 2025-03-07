#ifndef __KXEMU_CPU_RISCV_CORE_H__
#define __KXEMU_CPU_RISCV_CORE_H__

#include "cpu/core.h"
#include "cpu/riscv/aclint.h"
#include "cpu/riscv/plic.h"
#include "cpu/word.h"
#include "cpu/riscv/csr.h"
#include "device/bus.h"

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
        AMO   = STORE | LOAD,
    };
    device::Bus *bus;
    device::AClint *aclint;
    device::PLIC *plic;
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
    
    word_t vaddr_translate_bare(word_t addr, MemType type, VMResult &result);
    template<unsigned int LEVELS, unsigned int PTESIZE, unsigned int VPNBITS>
    word_t vaddr_translate_sv(word_t addr, MemType type, VMResult &result); // The template function for sv32, sv39, sv48, sv57
    #ifdef KXEMU_ISA32
    word_t vaddr_translate_sv32(word_t addr, MemType type, VMResult &result);
    #else
    word_t vaddr_translate_sv39(word_t addr, MemType type, VMResult &result);
    word_t vaddr_translate_sv48(word_t addr, MemType type, VMResult &result);
    word_t vaddr_translate_sv57(word_t addr, MemType type, VMResult &result);
    #endif
    
    word_t satpPPN;
    word_t (RVCore::*vaddr_translate_func)(word_t addr, MemType type, VMResult &result);
    void update_satp();
    void update_vm_addr_space();

    // Physical memory protection
    bool check_pmp(word_t addr, int len, MemType type);

    // Decoder
    struct DecodeInfo {
        unsigned int rd;
        unsigned int rs1;
        union {
            unsigned int rs2;
            unsigned int csr;
        };
        union {
            word_t imm;
            word_t npc;
        };

    #ifdef CONFIG_DEBUG_DECODER
        bool rd_set;
        bool rs1_set;
        bool rs2_set;
        bool csr_set;
        bool imm_set;
        bool npc_set;
    #endif
    };
    DecodeInfo gDecodeInfo;
    
    typedef void (RVCore::*do_inst_t)(const DecodeInfo &decodeInfo);
    do_inst_t decode_and_exec  ();
    do_inst_t decode_and_exec_c(); // for compressed instructions

    #include "./local-include/decode-list.h"

    // running
    uint32_t inst;
    word_t pc;
    word_t npc;
    void execute();

    // Trap
    void trap(word_t code, word_t value = 0);
    
    // Interrupt
    void   set_interrupt(word_t code);
    void clear_interrupt(word_t code);
    bool scan_interrupt();
    void interrupt_m(word_t code);
    void interrupt_s(word_t code);

    // Do instructions
    void do_invalid_inst();
    void do_invalid_inst(const DecodeInfo &decodeInfo);
    #include "./local-include/inst-list.h"

    // Zicsr and Privilleged
    RVCSR csr;
    int &privMode = csr.privMode; // Priviledge mode
    void init_csr();
    word_t  read_csr(unsigned int addr, bool &valid);
    bool   write_csr(unsigned int addr, word_t value);
    word_t get_csr_core(unsigned int addr);
    void   set_csr_core(unsigned int addr, word_t value);
    
    void update_mstatus();
    struct MstatusField {
        bool mie;
        bool sie;
        bool sum;
    } mstatus;

    const word_t *mie;
    const word_t *mip;
    const word_t *medeleg;
    const word_t *medelegh;
    const word_t *mideleg;

    void set_priv_mode(int mode);

    uint64_t get_uptime();
    void update_stimecmp();

    // Atomic extension
    std::unordered_map<word_t, word_t> reservedMemory; // for lr, sc
    word_t amo_vaddr_translate_and_set_trap(word_t vaddr, int len, bool &valid);
    template<int len> void do_load_reserved(const DecodeInfo &decodeInfo);
    template<int len> void do_store_conditional(const DecodeInfo &deocdeInfo);
    template<device::AMO amo, typename sw_t = int32_t> void do_amo_inst(const DecodeInfo &deocdeInfo);

    // Experimental ICache
    #ifdef CONFIG_ICache
    static constexpr unsigned int ICACHE_SET_BITS = 11;
    struct ICacheBlock {
        bool valid;
        word_t tag;
        
        do_inst_t do_inst;
        DecodeInfo decodeInfo;
        unsigned int instLen;
    };
    ICacheBlock icache[1 << ICACHE_SET_BITS];
    #endif
    void icache_push(do_inst_t do_inst, unsigned int instLen);
    bool icache_decode_and_exec();
    void icache_fence();
    
    word_t gpr[33];

    // Experimental DCache
    #ifdef CONFIG_DCache
    static constexpr unsigned int DCACHE_BLOCK_BITS = 10;
    static constexpr unsigned int DCACHE_BLOCK_SIZE = 1 << DCACHE_BLOCK_BITS;
    static constexpr unsigned int DCACHE_SET_BITS = 5;
    struct DCacheBlock {
        bool valid;
        bool dirty;
        word_t tag;
        void *raw;
        __attribute__((aligned(sizeof(word_t)))) 
        uint8_t data[DCACHE_BLOCK_SIZE];
        
        #ifdef CONFIG_DEBUG
        word_t addr;
        #endif
    };
    DCacheBlock dcache[1 << DCACHE_SET_BITS];
    DCacheBlock *dcache_hit(word_t addr, int len);
    bool dcache_load (word_t addr, int len, word_t &data);
    bool dcache_store(word_t addr, word_t data, int len);
    #endif

public:
    RVCore();
    ~RVCore();

    void init(unsigned int coreID, device::Bus *bus, int flags, device::AClint *aclint, device::PLIC *plic);
    
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
