#ifndef __KXEMU_RISCV_CSR_DEF_H__
#define __KXEMU_RISCV_CSR_DEF_H__

#include "config/config.h"
#include "word.h"

// AClint
#define ACLINT_BASE   0x02000000
#define ACLINT_SIZE   0x00010000
#define MSWI_BASE     0x0000
#define MSWI_SIZE     0x4000
#define MTIMECMP_BASE 0x4000
#define MTIMECMP_SIZE 0x4000
#define MTIME_BASE    0xbff8
#define MTIME_SIZE    0x0008
#define SSWI_BASE     0xc000
#define SSWI_SIZE     0x4000

#define CSR_READ_ONLY  0b110000000000
#define IS_CSR_READ_ONLY(addr) ((addr & CSR_READ_ONLY) == CSR_READ_ONLY)

// CSR Address
// Machine Information Registers
#define CSR_MVENDORID 0xf11
#define CSR_MARCHID   0xf12
#define CSR_MIMPID    0xf13
#define CSR_MHARTID   0xf14
#define CSR_MCFGPTR   0xf15

// Machine Trap Setup
#define CSR_MSTATUS 0x300
#define CSR_MISA    0x301
#define CSR_MEDELEG 0x302
#define CSR_MIDELEG 0x303
#define CSR_MIE     0x304
#define CSR_MTVEC   0x305
#define CSR_MCNTEN  0x306
#ifdef KXEMU_ISA32
    #define CSR_MSTATUSH 0x310
    #define CSR_MEDELEGH 0x312
#endif

// Machine Trap Handling
#define CSR_MSCRATCH 0x340
#define CSR_MEPC     0x341
#define CSR_MCAUSE   0x342
#define CSR_MTVAL    0x343
#define CSR_MIP      0x344
#define CSR_MTINST   0x34a
#define CSR_MTVAL2   0x34b

// Machine Configuration
#define CSR_MENVCFG 0x30a
#define CSR_MSECCFG 0x747
#ifdef KXEMU_ISA32
    #define CSR_MENVCFGH 0x31a
    #define CSR_MSECCFGH 0x757
#endif

// Machine Memory Protection
#define CSR_PMPCFG0  0x3a0
#define CSR_PMPADDR0 0x3b0

// Supervisor Trap Setup
#define CSR_SSTATUS 0x100
#define CSR_SIE     0x104
#define CSR_STVEC   0x105
#define CSR_SCNTEN  0x106

// Supervisor Trap Handling
#define CSR_SSCRATCH 0x140
#define CSR_SEPC     0x141
#define CSR_SCAUSE   0x142
#define CSR_STVAL    0x143
#define CSR_SIP      0x144
#define CSR_STIMECMP 0x14d // SSTC extension
#ifdef KXEMU_ISA32
    #define CSR_STIMECMPH 0x15d
#endif

// Supervisor Protection and Translation
#define CSR_SATP  0x180

// Unprivileged Counter/Timers
#define CSR_CYCLE 0xc00
#define CSR_TIME  0xc01
#ifdef KXEMU_ISA32
    #define CSR_CYCLEH 0xc80
    #define CSR_TIMEH  0xc81
#endif

#define MISA_A (1 <<  0) // Atomic Extension
#define MISA_B (1 <<  1) // B Extesnion
#define MISA_C (1 <<  2) // Compressed Extension
#define MISA_D (1 <<  3) // Double-precision Extension
#define MISA_E (1 <<  4) // RV32E/RV64E Base ISA
#define MISA_F (1 <<  5) // Single-precision Extension
#define MISA_H (1 <<  7) // Hypervisor Extension
#define MISA_I (1 <<  8) // RV32I/RV64I/RV128I Base ISA
#define MISA_M (1 << 12) // Integer Mutiply/Divide Extension
#define MISA_Q (1 << 16) // Quad-precision floating-point Extension
#define MISA_S (1 << 18) // Supervisor mode Implemented
#define MISA_U (1 << 20) // User mode Implemented
#define MISA_V (1 << 21) // Vector Extension

#define STATUS_SIE_OFF    1
#define STATUS_MIE_OFF    3
#define STATUS_SPIE_OFF   5
#define STATUS_UBE_OFF    6
#define STATUS_MPIE_OFF   7
#define STATUS_SPP_OFF    8
#define STATUS_VS_OFF     9
#define STATUS_MPP_OFF   11
#define STATUS_FS_OFF    13
#define STATUS_XS_OFF    15
#define STATUS_MPRV_OFF  17
#define STATUS_SUM_OFF   18
#define STATUS_MXR_OFF   19
#define STATUS_TVM_OFF   20
#define STATUS_TW_OFF    21
#define STATUS_TSR_OFF   22
#define STATUS_SPELP_OFF 23
#define STATUS_SDT_OFF   24

#ifdef KXEMU_ISA64
    #define STATUS_UXL_OFF   32
    #define STATUS_SXL_OFF   34
    #define STATUS_SBE_OFF   36
    #define STATUS_MBE_OFF   37
    #define STATUS_GVA_OFF   38
    #define STATUS_MPV_OFF   39
    #define STATUS_MPELP_OFF 41
    #define STATUS_MDT_OFF   42
    #define STATUS_SD_OFF    63
#else
    #define STATUS_SD_OFF    31
    // STATUS High
    #define STATUS_SBE_OFF    4
    #define STATUS_MBE_OFF    5
    #define STATUS_GVA_OFF    6
    #define STATUS_MPV_OFF    7
    #define STATUS_MPELP_OFF  9
    #define STATUS_MDT_OFF   10
#endif

#define STATUS_SIE_MASK  (1 << 1)
#define STATUS_SPIE_MASK (1 << 5)

#define STATUS_SPP_MASK (1 << STATUS_SPP_OFF)

#define STATUS_MIE_MASK  (1 << STATUS_MIE_OFF)
#define STATUS_MPIE_MASK (1 << STATUS_MPIE_OFF)
#define STATUS_MPP_MASK  (3 << STATUS_MPP_OFF)
#define STATUS_MPRV_MASK (1 << STATUS_MPRV_OFF)

#define STATUS_SUM(status) ((status & (1 << STATUS_SUM_OFF)) >> STATUS_SUM_OFF)

#define MCNTEN_CY_MASK (1 << 0)
#define MCNTEN_TM_MASK (1 << 1)
#define MCNTEN_IR_MASK (1 << 2)

#define MCNTEN_CY(mcounteren) ((mcounteren) & MCNTEN_CY_MASK)
#define MCNTEN_TM(mcounteren) ((mcounteren) & MCNTEN_TM_MASK)
#define MCNTEN_IR(mcounteren) ((mcounteren) & MCNTEN_IR_MASK)

#ifdef KXEMU_ISA64
    #define MENVCFG_STCE_MASK (1ULL << 63)
#else
    #define MENVCFG_STCE_MASK (1 << 31)
#endif
#define MENVCFG_STCE(menvcfg) ((menvcfg) & (MENVCFG_STCE_MASK))

#define PMPCFG_R_OFF 0
#define PMPCFG_W_OFF 1
#define PMPCFG_X_OFF 2
#define PMPCFG_A_OFF 3

#define PMPCFG_R_MASK (1 << PMPCFG_R_OFF)
#define PMPCFG_W_MASK (1 << PMPCFG_W_OFF)
#define PMPCFG_X_MASK (1 << PMPCFG_X_OFF)
#define PMPCFG_A_MASK (3 << PMPCFG_A_OFF)

#define PMPCONFIG_A_OFF   0
#define PMPCONFIG_A_TOR   1
#define PMPCONFIG_A_NA4   2
#define PMPCONFIG_A_NAPOT 3

#ifdef KXEMU_ISA64
    #define CAUSE_INTERRUPT_OFF 63
#else
    #define CAUSE_INTERRUPT_OFF 31
#endif
#define CAUSE_INTERRUPT_MASK (1ULL << CAUSE_INTERRUPT_OFF)

// Virtual Address Translation
#define SATP_PPN_OFF 0

#ifdef KXEMU_ISA64
    #define SATP_ASID_OFF 44
    #define SATP_MODE_OFF 60
    #define SATP_PPN_MASK ((1ULL << 44) - 1)
    #define SATP_ASID_MASK (0xffffULL << SATP_ASID_OFF)
    #define SATP_MODE_MASK (0xfULL << SATP_MODE_OFF)
#else
    #define SATP_ASID_OFF 22
    #define SATP_MODE_OFF 31
    #define SATP_PPN_MASK  ((1ULL << 22) - 1)
    #define SATP_ASID_MASK (0x1ff << SATP_ASID_OFF)
    #define SATP_MODE_MASK (0x1 << SATP_MODE_OFF)
#endif

#define SATP_PPN(satp) (satp & SATP_PPN_MASK)
#define SATP_MODE(satp) ((satp & SATP_MODE_MASK) >> SATP_MODE_OFF)
#define SATP_MODE_BARE  0
#define SATP_MODE_SV32  1
#define SATP_MODE_SV39  8
#define SATP_MODE_SV48  9
#define SATP_MODE_SV57 10
#define SATP_MODE_SV64 11

#define PTE_V_MASK  (1 << 0)
#define PTE_R_MASK  (1 << 1)
#define PTE_W_MASK  (1 << 2)
#define PTE_X_MASK  (1 << 3)
#define PTE_U_MASK  (1 << 4)
#define PTE_A_MASK  (1 << 6)

#define PTE_V(pte) ((pte & PTE_V_MASK) >> 0)
#define PTE_R(pte) ((pte & PTE_R_MASK) >> 1)
#define PTE_W(pte) ((pte & PTE_W_MASK) >> 2)
#define PTE_X(pte) ((pte & PTE_X_MASK) >> 3)
#define PTE_U(pte) ((pte & PTE_U_MASK) >> 4)
#define PTE_A(pte) ((pte & PTE_A_MASK) >> 6)

// Exception Code
// INTERRUPT
#define INTERRUPT_SOFTWARE_S   0
#define INTERRUPT_SOFTWARE_M   3
#define INTERRUPT_TIMER_S      5
#define INTERRUPT_TIMER_M      7
#define INTERRUPT_EXTERNAL_S   9
#define INTERRUPT_EXTERNAL_M  11
#define INTERRUPT_COUNTER     13

// TRAP
#define TRAP_INST_ADDR_MISALIGNED   0
#define TRAP_INST_ACCESS_FAULT      1
#define TRAP_ILLEGAL_INST           2
#define TRAP_BREAKPOINT             3
#define TRAP_LOAD_ADDR_MISALIGNED   4
#define TRAP_LOAD_ACCESS_FAULT      5
#define TRAP_STORE_ADDR_MISALIGNED  6
#define TRAP_AMO_ACCESS_MISALIGNED  6
#define TRAP_STORE_ACCESS_FAULT     7
#define TRAP_AMO_ACCESS_FAULT       7
#define TRAP_ECALL_U                8
#define TRAP_ECALL_S                9
#define TRAP_ECALL_M               11
#define TRAP_INST_PAGE_FAULT       12
#define TRAP_LOAD_PAGE_FAULT       13
#define TRAP_STORE_PAGE_FAULT      15
#define TRAP_AMO_PAGE_FAULT        15
#define TRAP_DOUBLE_TRAP           16
#define TRAP_SOFTWARE_CHECK        18
#define TRAP_HARDWARE_ERROR        19

// Machine or Supervisor Trap-Vector Mode
#define TVEC_MODE_MASK     (3 << 0)
#define TVEC_MODE_DIRECT   0x0
#define TVEC_MODE_VECTORED 0x1

// #define SSTATUS_MASK 0b10000000000011011110011101100010
#ifdef KXEMU_ISA64
#define SSTATUS_MASK \
( \
    (1ULL << STATUS_SIE_OFF)   | \
    (1ULL << STATUS_SPIE_OFF)  | \
    (1ULL << STATUS_SPP_OFF)   | \
    (3ULL << STATUS_FS_OFF)    | \
    (3ULL << STATUS_XS_OFF)    | \
    (1ULL << STATUS_SUM_OFF)   | \
    (1ULL << STATUS_MXR_OFF)   | \
    (1ULL << STATUS_SPELP_OFF) | \
    (1ULL << STATUS_SDT_OFF)   | \
    (3ULL << STATUS_UXL_OFF)   | \
    (1ULL << STATUS_SD_OFF)      \
)
#else
#define SSTATUS_MASK \
( \
    (1 << STATUS_SIE_OFF)   | \
    (1 << STATUS_SPIE_OFF)  | \
    (1 << STATUS_SPP_OFF)   | \
    (3 << STATUS_FS_OFF)    | \
    (3 << STATUS_XS_OFF)    | \
    (1 << STATUS_SUM_OFF)   | \
    (1 << STATUS_MXR_OFF)   | \
    (1 << STATUS_SPELP_OFF) | \
    (1 << STATUS_SDT_OFF)   | \
    (1 << STATUS_SD_OFF)      \
)
#endif

#define UPTIME_TO_MTIME(uptime) (uptime / 100)
#define MTIME_TO_UPTIME(mtime)  (mtime * 100)

namespace kxemu::cpu {

enum PrivMode {
    MACHINE = 3,
    SUPERVISOR = 1,
    USER = 0,
};

}

#endif
