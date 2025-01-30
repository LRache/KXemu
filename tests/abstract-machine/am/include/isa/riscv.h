#ifndef __ISA_H__
#define __ISA_H__

#include <stdint.h>

#define NR_REGS 32

#ifdef __riscv_e
#define GPR1 gpr[15] // a5
#else
#define GPR1 gpr[17] // a7
#endif

#define GPR2 gpr[10]
#define GPR3 gpr[11]
#define GPR4 gpr[12]
#define GPRx gpr[10]

#define IRQ_TIMER 0x80000007 // timer interrupt

#define PTE_V (1 << 0)
#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_U (1 << 4)

#define VM_SV32 (1 << 31)
#define VM_SV39 (8ULL << 60)
#define VM_SV48 (9ULL << 60)
#define VM_SV57 (10ULL << 60)

#define MSTATUS_MPP_MASK 0x00001800
#define MSTATUS_MPP_U 0

typedef uintptr_t word_t;

word_t csrr_mstatus();
word_t csrr_mcause ();
word_t csrr_mepc   ();
word_t csrr_medeleg();

void csrw_pmpcfg0(word_t value);
void csrw_pmpaddr0(word_t value);

void csrw_mstatus(word_t value);
void csrw_mcause (word_t value);
void csrw_mepc   (word_t value);
void csrw_mtvec  (word_t value);
void csrw_medeleg(word_t value);
void csrw_mscratch(word_t value);

void csrw_sepc   (word_t value);
void csrw_stvec  (word_t value);
void csrw_satp   (word_t value);

void mret();
void sret();
void ecall();
void wfi();

void sfence_vma();

// CTE
typedef struct Context {
    uintptr_t gpr[32];
} Context;

typedef struct {
    enum {
        EVENT_NULL = 0,
        EVENT_SYSCALL, EVENT_PAGEFAULT, EVENT_ERROR, EVENT_IRQ_TIMER, EVENT_IRQ_IODEV,
    } event;
    uintptr_t cause;
} Event;

void cte_init(Context*(*handler)(Event, Context*));
void yield();

#endif
