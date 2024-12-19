#ifndef __ISA_RISCV32_CSR_DEF_H__
#define __ISA_RISCV32_CSR_DEF_H__

#define CSR_READ_ONLY  0b110000000000
#define IS_CSR_READ_ONLY(addr) ((addr & CSR_READ_ONLY) == CSR_READ_ONLY)

#define CSR_MISA   0xF10
#define CSR_MTVEC  0x305
#define CSR_MEPC   0x341
#define CSR_MCAUSE 0x342

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

#define MSTATUS_SIE  (1 << 1)
#define MSTATUS_MIE  (1 << 3)
#define MSTATUS_MPIE (1 << 7)
#define MSTATUS_MPRV (1 << 17)

#endif
