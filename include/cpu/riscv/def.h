#ifndef __KXEMU_CPU_RISCV_NAMESPACE_H__
#define __KXEMU_CPU_RISCV_NAMESPACE_H__

#include "cpu/word.h"
#include "device/def.h"
#include "config/config.h"

namespace kxemu::cpu {
    static inline constexpr unsigned int ICACHE_SET_BITS = 11;
    static inline constexpr unsigned int TLB_SET_BITS = 5;

    static inline constexpr unsigned int PGBITS = 12;
    static inline constexpr word_t PGSIZE = (1 << PGBITS);

    enum PrivMode {
        MACHINE    = 3,
        SUPERVISOR = 1,
        USER       = 0,   
    };

    enum CSRAddr : unsigned int {
        // Machine Information Registers
        MVENDORID = 0xf11,
        MARCHID   = 0xf12,
        MIMPID    = 0xf13,
        MHARTID   = 0xf14,
        MCFGPTR   = 0xf15,
        
        // Machine Trap Setup
        MSTATUS   = 0x300,
        MISA      = 0x301,
        MEDELEG   = 0x302,
        MIDELEG   = 0x303,
        MIE       = 0x304,
        MTVEC     = 0x305,
        MCNTEN    = 0x306,
        #ifdef KXEMU_ISA32
        MSTATUSH  = 0x310,
        MEDELEGH  = 0x312,
        #endif

        // Machine Trap Handling
        MSCRATCH  = 0x340,
        MEPC      = 0x341,
        MCAUSE    = 0x342,
        MTVAL     = 0x343,
        MIP       = 0x344,
        MTINST    = 0x34a,
        MTVAL2    = 0x34b,

        // Machine Configuration
        MENVCFG   = 0x30a,
        MSECCFG   = 0x747,
        #ifdef KXEMU_ISA32
        MENVCFGH  = 0x31a,
        MSECCFGH  = 0x757,
        #endif

        // Machine Memory Protection
        PMPCFG0   = 0x3a0,
        PMPADDR0  = 0x3b0,

        // Supervisor Trap Setup
        SSTATUS   = 0x100,
        SIE       = 0x104,
        STVEC     = 0x105,
        SCNTEN    = 0x106,

        // Supervisor Trap Handling
        SSCRATCH  = 0x140,
        SEPC      = 0x141,
        SCAUSE    = 0x142,
        STVAL     = 0x143,
        SIP       = 0x144,
        STINST    = 0x14a,
        STVAL2    = 0x14b,
        STIMECMP  = 0x14d, // SSTC extension
        #ifdef KXEMU_ISA32
        STIMECMPH = 0x15d,
        #endif

        // Supervisor Memory Protection
        SATP   = 0x180,
        
        // Unprivileged Floating-Point CSRs
        FFLAGS = 0x001,
        FRM    = 0x002,
        FCSR   = 0x003,

        // Unprivileged Counter/Timers
        CYCLE  = 0xc00,
        TIME   = 0xc01,
        #ifdef KXEMU_ISA32
        CYCLEH = 0xc80,
        TIMEH  = 0xc81,
        #endif
    };

    enum InterruptCode : word_t {
        SOFTWARE_S = 0,
        SOFTWARE_M = 3,
        TIMER_S    = 5,
        TIMER_M    = 7,
        EXTERNAL_S = 9,
        EXTERNAL_M = 11,
        COUNTER    = 13,
    };

    enum TrapCode : word_t {
        INST_ADDR_MISALIGNED  = 0,
        INST_ACCESS_FAULT     = 1,
        ILLEGAL_INST          = 2,
        BREAKPOINT            = 3,
        LOAD_ADDR_MISALIGNED  = 4,
        LOAD_ACCESS_FAULT     = 5,
        STORE_ADDR_MISALIGNED = 6,
        AMO_ACCESS_MISALIGNED = 6,
        STORE_ACCESS_FAULT    = 7,
        AMO_ACCESS_FAULT      = 7,
        ECALL_U               = 8,
        ECALL_S               = 9,
        ECALL_M               = 11,
        INST_PAGE_FAULT       = 12,
        LOAD_PAGE_FAULT       = 13,
        STORE_PAGE_FAULT      = 15,
        AMO_PAGE_FAULT        = 15,
        DOUBLE_TRAP           = 16,
        SOFTWARE_CHECK        = 18,
        HARDWARE_ERROR        = 19,
    };

    static inline bool csr_read_only(unsigned int addr) {
        return (addr & 0b110000000000) == 0b110000000000;
    }

    static inline word_t realtime_to_mtime(word_t uptime) {
        return uptime / 100;
    }

    static inline word_t mtime_to_realtime(word_t mtime) {
        return mtime * 100;
    }

    inline constexpr device::AddrSpace ACLINT   = {0x02000000, 0x00010000};
    inline constexpr device::AddrSpace PLIC     = {0x0c000000, 0x4000000};

    class RVCore;

    inline constexpr word_t KXEMU_VENDORID = 0x584b5343; // "CSKX" in little-endian
    inline constexpr word_t KXEMU_ARCHID   = 0x00CAFFEE;
}

#endif
