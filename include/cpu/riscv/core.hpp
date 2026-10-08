#ifndef __KXEMU_CPU_RISCV_CORE_HPP__
#define __KXEMU_CPU_RISCV_CORE_HPP__

#include "cpu/core.hpp"
#include "cpu/riscv/aclint.hpp"
#include "cpu/riscv/addr.hpp"
#include "cpu/riscv/def.hpp"
#include "cpu/riscv/plic.hpp"
#include "cpu/riscv/pte.hpp"
#include "cpu/word.hpp"
#include "cpu/riscv/csr.hpp"
#include "cpu/riscv/config.hpp"
#include "device/bus.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
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
    };
    std::atomic<state_t> state{IDLE};

    word_t haltCode;
    word_t haltPC;

    class TrapException : public std::exception {
    private:
        TrapCode code_;
        word_t value_;
    public:
        TrapException(TrapCode code, word_t value = 0) : code_(code), value_(value) {}
        TrapCode code() const { return code_; }
        word_t value() const { return value_; }
        const char *what() const noexcept override {
            return "Trap Exception";
        }
    };

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

    std::optional<mem_value_t> pm_read_check_optional(word_t paddr, unsigned int len);

    void        memory_fetch();
    mem_value_t memory_load (word_t addr, unsigned int len);
    void        memory_store(word_t addr, mem_value_t data, unsigned int len);

    void        pm_fetch(word_t paddr);
    mem_value_t pm_read(word_t paddr, unsigned int len);
    void        pm_write(word_t paddr, mem_value_t data, unsigned int len);
    
    mem_value_t pm_read_check (word_t paddr, unsigned int len); // With PMP check
    void        pm_write_check(word_t paddr, mem_value_t data, unsigned int len);

    // Virtual address translation
    void        vm_fetch();
    mem_value_t vm_read (word_t vaddr, unsigned int len);
    void        vm_write(word_t vaddr, mem_value_t data, unsigned int len);

    enum class VMFault {
        PAGE_FAULT,
        ACCESS_FAULT,
    };
    using VMResult = std::expected<word_t, VMFault>;
    VMResult vaddr_translate_core(addr_t addr, MemType type) noexcept;
    
    template<unsigned int LEVELS, unsigned int PTESIZE, unsigned int VPNBITS>
    VMResult vaddr_translate_sv(addr_t vaddr, MemType type) noexcept; // The template function for sv32, sv39, sv48, sv57
    
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
    void enter_trap(TrapCode code, word_t value = 0);
    
    // Interrupt
    // Producers update mip directly. wakePending is guarded by csr.pendingMtx
    // (see RVCSR::lock_pending) so a level change and its wake are published together.
    std::condition_variable interruptReady;
    bool wakePending = false;
    std::atomic<bool> stopping{false};

    enum class WaitResult {
        Notified, // A notification does not imply WFI or trap eligibility.
        Timeout,  // Allow legacy devices to be polled.
        Stopped,  // Persists until reset().
    };
    /// Update IRQ levels under the pending CSR lock, then notify the hart.
    void set_interrupts(word_t mask, bool asserted);
    void   set_interrupt(InterruptCode code);
    void clear_interrupt(InterruptCode code);

    /// Wait for a notification, stop request or timeout on the owning hart.
    /// The default waits indefinitely; zero/negative durations only check current state.
    /// WFI and trap eligibility are checked separately by the execution path.
    WaitResult wait_for_interrupt(
        std::chrono::nanoseconds timeout = std::chrono::nanoseconds::max());
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
    RVCSR::ModifyResult modify_csr(unsigned int addr, word_t operand, RVCSR::ModifyOp op, bool readOld = true);
    void apply_csr_effect(RVCSR::WriteEffect effect);
    // Shared body of the Zicsr instructions; raises illegal instruction on failure.
    void execute_csr(const DecodeInfo &decodeInfo, word_t operand, RVCSR::ModifyOp op, bool readOld = true);
    
    std::mutex csrMtx;

    void set_priv_mode(int mode);

    uint64_t get_uptime();
    void update_stimecmp();

    // Atomic extension
    std::unordered_map<word_t, word_t> reservedMemory; // for lr, sc
    word_t amo_vaddr_translate_and_set_trap(word_t vaddr, int len);
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
            float f32;    // low 32 bits
            uint32_t high; // high 32 bits, filled with 1s for NaN boxing
        };
    } fpr[32];
    void raise_fflags(word_t flags);

    // Experimental ICache
    #ifdef CONFIG_ENABLE_ICACHE
    struct ICacheBlock {
        word_t tag;
        
        do_inst_t do_inst;
        DecodeInfo decodeInfo;
        unsigned int instLen;
        bool valid = false;
    };
    ICacheBlock icache[1 << config::ICACHE_SET_BITS];
    #endif
    void icache_push(do_inst_t do_inst, unsigned int instLen, const DecodeInfo &decodeInfo);
    bool icache_decode_and_exec();
    void icache_fence();

    struct TLBBlock {
        word_t paddr;
        word_t tag;
        word_t pteAddr;
        bool valid = false;
        PTEFlag flag;
    };
    TLBBlock tlb[1 << config::TLB_SET_BITS];
    void tlb_push(addr_t vaddr, addr_t paddr, word_t pteAddr, uint8_t type);
    std::optional<TLBBlock *> tlb_hit(addr_t vaddr);
    void tlb_fence();

public:
    RVCore();
    ~RVCore();

    void init(unsigned int coreID, device::Bus *bus, device::AClint *aclint, device::PLIC *plic);
    
    /// Reset CPU and IRQ/wait state only after the hart and all producers are quiescent.
    void reset(word_t entry) override;
    void step() override;
    void run(const word_t *breakpoints = nullptr, unsigned int n = 0) override;
    void set_device_mtx(std::mutex *mtx);

    /// Notify the hart to recheck state without modifying IRQ levels.
    /// May be called by any producer; notifications persist until consumed and coalesce.
    /// IRQ injection must use the set/clear interrupt methods, not wake() alone.
    void wake();

    /// Request stop and wake the hart. May be repeated; does not join its host thread.
    /// All producers and the hart must finish accessing this Core before destruction.
    void request_stop() override;

    /// Query the persistent stop flag from any thread without consuming updates.
    bool stop_requested() const {
        return stopping.load(std::memory_order_acquire);
    }

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
