#ifndef __KXEMU_CPU_RISCV_CSR_HPP__
#define __KXEMU_CPU_RISCV_CSR_HPP__

#include "cpu/riscv/def.hpp"
#include "cpu/riscv/csr-field.hpp"
#include "cpu/word.hpp"

#include <functional>
#include <optional>
#include <cstdint>
#include <atomic>
#include <mutex>

namespace kxemu::cpu {

class RVCore;

// Architectural CSR storage. Default member initializers are the reset values,
// so RVCSR::reset() is a single assignment; MHARTID is patched in afterwards.
struct RVCSRState {
    // Interrupt state has direct storage; supervisor aliases are views only.
    // mip is guarded by RVCSR::pendingMtx; the rest is owned by the hart thread.
    word_t mip = 0;
    word_t mie = 0;
    csr::MStatus mstatus{word_t{0x1800}};
    word_t mideleg = 0;
    word_t medeleg = 0;
#ifdef KXEMU_ISA32
    word_t medelegh = 0;
#endif

    // Read-only machine information.
    word_t mvendorid = KXEMU_VENDORID;
    word_t marchid = KXEMU_ARCHID;
    word_t mimpid = 0, mhartid = 0, mcfgptr = 0;

    // Writable configuration and scratch state.
    word_t misa = csr::MISA();
    word_t mcnten = 0, scnten = 0;
    word_t menvcfg = 0, mseccfg = 0;
    word_t mscratch = 0, sscratch = 0;
    csr::Satp satp{word_t{0}}; // Guest writes validate MODE and mask ASID.
    word_t stimecmp = 0;
    // CYCLE is currently a placeholder; TIME reads obtain the current time.
    word_t cycle = 0;
#ifdef KXEMU_ISA32
    word_t mstatush = 0, menvcfgh = 0, mseccfgh = 0;
    word_t stimecmph = 0, cycleh = 0;
#endif

    // Trap CSRs are read and written together on exception/interrupt entry.
    word_t mtvec = 0, stvec = 0;
    word_t mepc = 0, sepc = 0;
    word_t mcause = 0, scause = 0;
    word_t mtval = 0, stval = 0;
    word_t mtinst = 0, stinst = 0, mtval2 = 0;

    // FFLAGS and FRM are views of FCSR.
    csr::FCSR fcsr{word_t{0}};

    // Each pmpcfg CSR is a packed view of four (RV32) or eight (RV64) items.
    csr::PMPCfgItem pmpcfg[64] = {};
    word_t pmpaddr[64] = {};
};

// Private inheritance keeps the storage names (csr.mstatus, ...) while hiding
// them from everyone except RVCSR and its friends.
class RVCSR : private RVCSRState {
public:
    enum class WriteEffect { None, VmTranslate, STimer };
    enum class ModifyOp { Read, Write, Set, Clear };
    struct ModifyResult {
        bool valid;
        word_t oldValue;
        WriteEffect effect;
    };

    // Proof that pendingMtx is held. Owns nothing for CSRs that do not need it.
    using PendingLock = std::unique_lock<std::mutex>;

private:
    // RVCore accesses CSR storage directly on its own hart thread.
    friend class RVCore;

    // init() supplies the reset value of MHARTID.
    word_t resetHartId = 0;

    // Guards mip and also RVCore::wakePending, so that an IRQ producer publishes
    // the new level and the wake flag atomically; RVCore::interruptReady waits on it.
    mutable std::mutex pendingMtx;
    // Cache of mip != 0; enable/delegation checks remain on the hart.
    std::atomic<bool> pendingHint{false};

    PendingLock lock_pending() const;
    void assert_pending_locked(const PendingLock &lock) const;
    // These also refresh pendingHint.
    void set_pending(const PendingLock &lock, word_t value);
    void update_pending(const PendingLock &lock, word_t mask, bool asserted);
    word_t read_mip() const;

    // Debug writes store raw values; guest writes apply WARL masks.
    enum class WriteMode { Guest, Debug };
    enum class WriteStatus { Invalid, Ignored, Written };
    struct WriteResult {
        WriteStatus status;
        WriteEffect effect = WriteEffect::None; // Meaningful only when Written.
    };
    static bool is_pending_csr(unsigned int addr);
    static bool is_pmpcfg_csr(unsigned int addr);
    static bool is_pmpaddr_csr(unsigned int addr);
    // Locks pendingMtx only when addr is MIP or SIP.
    PendingLock lock_for(unsigned int addr) const;
    // lock must come from lock_for(addr).
    std::optional<word_t> read_csr(unsigned int addr, const PendingLock &lock) const;
    WriteResult write_csr(unsigned int addr, word_t value, WriteMode mode, const PendingLock &lock);

    struct PMPCfg {
        uint64_t start;
        uint64_t end;
        bool r;
        bool w;
        bool x;
    } pmpCfgArray[64];
    unsigned int pmpCfgCount = 0;
    void reload_pmpcfg();
    PMPCfg *pmp_check(word_t addr, int len);

    word_t read_pmpcfg(unsigned int addr) const;
    void write_pmpcfg (unsigned int addr, word_t value);
    void write_pmpaddr(unsigned int addr, word_t value);

    std::function<uint64_t()> get_uptime;
    bool time_readable() const;
    std::optional<word_t> read_time() const;
#ifdef KXEMU_ISA32
    std::optional<word_t> read_timeh() const;
#endif

public:
    unsigned int privMode;

    void init(unsigned int hartId, std::function<uint64_t()> get_uptime);
    void reset();

    bool has_pending_interrupt() const;
    uint64_t stimecmp_value() const;

    // Guest CSR operation: MIP/SIP hold pendingMtx across the entire RMW;
    // other state is owned by the hart thread. The core applies returned effects.
    // readOld=false suppresses the read only for Write (CSRRW[I] with rd=x0).
    ModifyResult modify_csr(unsigned int addr, word_t operand, ModifyOp op, bool readOld = true);

    // Debugger access: no privilege checks and no WARL masking beyond what keeps
    // the state representable. A failed write leaves the CSR unchanged.
    std::optional<word_t> debug_read_csr(unsigned int addr) const;
    std::optional<WriteEffect> debug_write_csr(unsigned int addr, word_t value);

    bool pmp_check_r(word_t addr, int len);
    bool pmp_check_w(word_t addr, int len);
    bool pmp_check_x(word_t addr, int len);
}; // class RVCSR

} // namespace kxemu::cpu

#endif
