#include "cpu/riscv/csr.hpp"
#include "cpu/riscv/csr-field.hpp"
#include "cpu/riscv/def.hpp"
#include "cpu/word.hpp"
#include "debug.h"

#include <optional>
#include <utility>

using namespace kxemu::cpu;

bool RVCSR::is_pending_csr(unsigned int addr) {
    return addr == CSRAddr::MIP || addr == CSRAddr::SIP;
}

bool RVCSR::is_pmpcfg_csr(unsigned int addr) {
    if (addr < CSRAddr::PMPCFG0 || addr >= CSRAddr::PMPCFG0 + 16) return false;
#ifdef KXEMU_ISA64
    return (addr - CSRAddr::PMPCFG0) % 2 == 0;
#else
    return true;
#endif
}

bool RVCSR::is_pmpaddr_csr(unsigned int addr) {
    return addr >= CSRAddr::PMPADDR0 && addr < CSRAddr::PMPADDR0 + 64;
}

bool RVCSR::has_pending_interrupt() const {
    return this->pendingHint.load(std::memory_order_relaxed);
}

uint64_t RVCSR::stimecmp_value() const {
    uint64_t value = this->stimecmp;
#ifdef KXEMU_ISA32
    value &= 0xffffffff;
    value |= uint64_t{this->stimecmph} << 32;
#endif
    return value;
}

RVCSR::PendingLock RVCSR::lock_pending() const {
    return PendingLock(this->pendingMtx);
}

RVCSR::PendingLock RVCSR::lock_for(unsigned int addr) const {
    return is_pending_csr(addr) ? this->lock_pending() : PendingLock();
}

void RVCSR::assert_pending_locked([[maybe_unused]] const PendingLock &lock) const {
    Assert(lock.owns_lock() && lock.mutex() == &this->pendingMtx,
           "pending CSR accessed without pendingMtx\n");
}

void RVCSR::set_pending(const PendingLock &lock, word_t value) {
    this->assert_pending_locked(lock);
    this->mip = value;
    this->pendingHint.store(value != 0, std::memory_order_relaxed);
}

void RVCSR::update_pending(const PendingLock &lock, word_t mask, bool asserted) {
    const word_t pending = this->mip;
    this->set_pending(lock, asserted ? pending | mask : pending & ~mask);
}

word_t RVCSR::read_mip() const {
    const auto lock = this->lock_pending();
    return this->mip;
}

std::optional<word_t> RVCSR::read_csr(unsigned int addr, const PendingLock &lock) const {
    switch (addr) {
        // Machine information
        case CSRAddr::MVENDORID: return this->mvendorid;
        case CSRAddr::MARCHID:   return this->marchid;
        case CSRAddr::MIMPID:    return this->mimpid;
        case CSRAddr::MHARTID:   return this->mhartid;
        case CSRAddr::MCFGPTR:   return this->mcfgptr;

        // Machine trap setup and handling
        case CSRAddr::MSTATUS:   return this->mstatus;
        case CSRAddr::MISA:      return this->misa;
        case CSRAddr::MEDELEG:   return this->medeleg;
        case CSRAddr::MIDELEG:   return this->mideleg;
        case CSRAddr::MIE:       return this->mie;
        case CSRAddr::MTVEC:     return this->mtvec;
        case CSRAddr::MCNTEN:    return this->mcnten;
        case CSRAddr::MSCRATCH:  return this->mscratch;
        case CSRAddr::MEPC:      return this->mepc;
        case CSRAddr::MCAUSE:    return this->mcause;
        case CSRAddr::MTVAL:     return this->mtval;
        case CSRAddr::MTINST:    return this->mtinst;
        case CSRAddr::MTVAL2:    return this->mtval2;
        case CSRAddr::MIP:
            this->assert_pending_locked(lock);
            return this->mip;
        case CSRAddr::MENVCFG:   return this->menvcfg;
        case CSRAddr::MSECCFG:   return this->mseccfg;
#ifdef KXEMU_ISA32
        case CSRAddr::MSTATUSH:  return this->mstatush;
        case CSRAddr::MEDELEGH:  return this->medelegh;
        case CSRAddr::MENVCFGH:  return this->menvcfgh;
        case CSRAddr::MSECCFGH:  return this->mseccfgh;
#endif

        // Supervisor
        case CSRAddr::SSTATUS:   return this->mstatus.sstatus();
        case CSRAddr::SIE:       return csr::MIE(this->mie).sie();
        case CSRAddr::STVEC:     return this->stvec;
        case CSRAddr::SCNTEN:    return this->scnten;
        case CSRAddr::SSCRATCH:  return this->sscratch;
        case CSRAddr::SEPC:      return this->sepc;
        case CSRAddr::SCAUSE:    return this->scause;
        case CSRAddr::STVAL:     return this->stval;
        case CSRAddr::STINST:    return this->stinst;
        case CSRAddr::SIP:
            this->assert_pending_locked(lock);
            return csr::MIP(this->mip).sip();
        case CSRAddr::STIMECMP:  return this->stimecmp;
        case CSRAddr::SATP:      return this->satp;
#ifdef KXEMU_ISA32
        case CSRAddr::STIMECMPH: return this->stimecmph;
#endif

        // Floating point
        case CSRAddr::FFLAGS:    return this->fcsr.fflags();
        case CSRAddr::FRM:       return this->fcsr.frm();
        case CSRAddr::FCSR:      return this->fcsr;

        // Counters and timers
        case CSRAddr::CYCLE:     return this->cycle;
        case CSRAddr::TIME:      return this->read_time();
#ifdef KXEMU_ISA32
        case CSRAddr::CYCLEH:    return this->cycleh;
        case CSRAddr::TIMEH:     return this->read_timeh();
#endif

        default:
            if (is_pmpcfg_csr(addr))  return this->read_pmpcfg(addr);
            if (is_pmpaddr_csr(addr)) return this->pmpaddr[addr - CSRAddr::PMPADDR0];
            return std::nullopt;
    }
}

RVCSR::WriteResult RVCSR::write_csr(unsigned int addr, word_t value, WriteMode mode, const PendingLock &lock) {
    const bool guest = mode == WriteMode::Guest;
    WriteEffect effect = WriteEffect::None;
    switch (addr) {
        // Machine trap setup and handling
        case CSRAddr::MSTATUS:   this->mstatus  = csr::MStatus(value); break;
        case CSRAddr::MISA:      this->misa     = value; break;
        case CSRAddr::MEDELEG:   this->medeleg  = value; break;
        case CSRAddr::MIDELEG:   this->mideleg  = value; break;
        case CSRAddr::MIE:       this->mie      = guest ? word_t(csr::MIE(value)) : value; break;
        case CSRAddr::MTVEC:     this->mtvec    = value; break;
        case CSRAddr::MCNTEN:    this->mcnten   = value; break;
        case CSRAddr::MSCRATCH:  this->mscratch = value; break;
        case CSRAddr::MEPC:      this->mepc     = value; break;
        case CSRAddr::MCAUSE:    this->mcause   = value; break;
        case CSRAddr::MTVAL:     this->mtval    = value; break;
        case CSRAddr::MTINST:    this->mtinst   = value; break;
        case CSRAddr::MTVAL2:    this->mtval2   = value; break;
        case CSRAddr::MIP: {
            csr::MIP pending(this->mip);
            if (guest) {
                pending.set_mip(value);
            } else {
                pending = value;
            }
            this->set_pending(lock, pending);
        } break;
        case CSRAddr::MENVCFG:   this->menvcfg  = value; break;
        case CSRAddr::MSECCFG:   this->mseccfg  = value; break;
#ifdef KXEMU_ISA32
        case CSRAddr::MSTATUSH:  this->mstatush = value; break;
        case CSRAddr::MEDELEGH:  this->medelegh = value; break;
        case CSRAddr::MENVCFGH:  this->menvcfgh = value; break;
        case CSRAddr::MSECCFGH:  this->mseccfgh = value; break;
#endif

        // Supervisor
        case CSRAddr::SSTATUS:   this->mstatus.set_sstatus(value); break;
        case CSRAddr::SIE: {
            csr::MIE enabled = this->mie;
            enabled.set_sie(value);
            this->mie = enabled;
        } break;
        case CSRAddr::STVEC:     this->stvec    = value; break;
        case CSRAddr::SCNTEN:    this->scnten   = value; break;
        case CSRAddr::SSCRATCH:  this->sscratch = value; break;
        case CSRAddr::SEPC:      this->sepc     = value; break;
        case CSRAddr::SCAUSE:    this->scause   = value; break;
        case CSRAddr::STVAL:     this->stval    = value; break;
        case CSRAddr::STINST:    this->stinst   = value; break;
        case CSRAddr::SIP: {
            csr::MIP pending(this->mip);
            pending.set_sip(value);
            this->set_pending(lock, pending);
        } break;
        case CSRAddr::STIMECMP:  this->stimecmp = value; effect = WriteEffect::STimer; break;
#ifdef KXEMU_ISA32
        case CSRAddr::STIMECMPH: this->stimecmph = value; effect = WriteEffect::STimer; break;
#endif
        case CSRAddr::SATP: {
            csr::Satp satp = value;
            // An unsupported MODE ignores the entire write, including ASID and PPN.
            if (!satp.valid()) return {WriteStatus::Ignored};
            // This implementation has ASIDLEN=0.
            if (guest) satp.set_asid(0);
            this->satp = satp;
            effect = WriteEffect::VmTranslate;
        } break;

        // Floating point
        case CSRAddr::FFLAGS:    this->fcsr.set_fflags(value); break;
        case CSRAddr::FRM:       this->fcsr.set_frm(value); break;
        case CSRAddr::FCSR:      this->fcsr.set_fcsr(value); break;

        default:
            if (is_pmpcfg_csr(addr)) {
                this->write_pmpcfg(addr, value);
            } else if (is_pmpaddr_csr(addr)) {
                this->write_pmpaddr(addr, value);
            } else {
                // Unknown, or read-only by implementation.
                return {WriteStatus::Invalid};
            }
            break;
    }
    return {WriteStatus::Written, effect};
}

RVCSR::ModifyResult RVCSR::modify_csr(unsigned int addr, word_t operand, ModifyOp op, bool readOld) {
    constexpr ModifyResult invalid{false, 0, WriteEffect::None};
    const bool needRead = readOld || op != ModifyOp::Write;
    const bool needWrite = op != ModifyOp::Read;
    // Reject before reading, so that a rejected write has no read effects.
    if (needWrite && csr_read_only(addr)) return invalid;

    // MIP/SIP keep the lock across the whole read-modify-write.
    const auto lock = this->lock_for(addr);

    word_t old = 0;
    if (needRead) {
        const auto value = this->read_csr(addr, lock);
        if (!value.has_value()) return invalid;
        old = *value;
    }
    if (!needWrite) return {true, old, WriteEffect::None};

    const word_t value = op == ModifyOp::Set   ? old |  operand
                       : op == ModifyOp::Clear ? old & ~operand
                       : operand;
    const auto result = this->write_csr(addr, value, WriteMode::Guest, lock);
    switch (result.status) {
        case WriteStatus::Invalid: return invalid;
        case WriteStatus::Ignored: return {true, old, WriteEffect::None};
        case WriteStatus::Written: return {true, old, result.effect};
    }
    std::unreachable();
}

std::optional<word_t> RVCSR::debug_read_csr(unsigned int addr) const {
    return this->read_csr(addr, this->lock_for(addr));
}

std::optional<RVCSR::WriteEffect> RVCSR::debug_write_csr(unsigned int addr, word_t value) {
    const auto result = this->write_csr(addr, value, WriteMode::Debug, this->lock_for(addr));
    if (result.status != WriteStatus::Written) return std::nullopt;
    return result.effect;
}

word_t RVCSR::read_pmpcfg(unsigned int addr) const {
    const unsigned int first = (addr - CSRAddr::PMPCFG0) * 4;
    word_t value = 0;
    for (unsigned int i = 0; i < sizeof(word_t); ++i) {
        value |= word_t{uint8_t(this->pmpcfg[first + i])} << (i * 8);
    }
    return value;
}

void RVCSR::write_pmpcfg(unsigned int addr, word_t value) {
    const unsigned int first = (addr - CSRAddr::PMPCFG0) * 4;
    for (unsigned int i = 0; i < sizeof(word_t); ++i) {
        this->pmpcfg[first + i] = csr::PMPCfgItem(uint8_t(value >> (i * 8)));
    }
    this->reload_pmpcfg();
}

void RVCSR::write_pmpaddr(unsigned int addr, word_t value) {
    this->pmpaddr[addr - CSRAddr::PMPADDR0] = csr::PMPAddr(value);
    this->reload_pmpcfg();
}

bool RVCSR::time_readable() const {
    // M-mode can always read TIME; only lower modes need counter/extension checks.
    if (this->privMode == PrivMode::MACHINE) return true;

    const csr::MCounteren mcnten = this->mcnten;
    switch (this->privMode) {
        case PrivMode::SUPERVISOR: {
#ifdef KXEMU_ISA32
            const csr::MEnvConfigH menvcfgh = this->menvcfgh;
            return mcnten.tm() && menvcfgh.stce();
#else
            const csr::MEnvConfig menvcfg = this->menvcfg;
            return mcnten.tm() && menvcfg.stce();
#endif
        }
        case PrivMode::USER: {
            const csr::MCounteren scnten = this->scnten;
            return mcnten.tm() && scnten.tm();
        }
        default: std::unreachable();
    }
}

std::optional<word_t> RVCSR::read_time() const {
    if (!this->time_readable()) {
        return std::nullopt;
    }

    uint64_t mtime = realtime_to_mtime(this->get_uptime());
    return mtime;
}

#ifdef KXEMU_ISA32
std::optional<word_t> RVCSR::read_timeh() const {
    if (!this->time_readable()) {
        return std::nullopt;
    }

    uint64_t mtime = realtime_to_mtime(this->get_uptime());
    return mtime >> 32;
}
#endif
