#include "isa/riscv.h"
#include "klib.h"

#define MSTATUS_MPP_MASK (3 << 11)
#define MSTATUS_MPP_S    (1 << 11)

void m_mode_entry() {
    printf("Entered M-mode\n");
    while (1);
}

void s_mode_entry() {
    printf("Entered S-mode\n");
    csrw_sepc((uint32_t)m_mode_entry);
    sret();
}

int main() {
    uint32_t mstatus = csrr_mstatus();
    mstatus = (mstatus & ~MSTATUS_MPP_MASK) | MSTATUS_MPP_S;
    csrw_mstatus(mstatus);
    csrw_mepc((uint32_t)s_mode_entry);
    mret();
    return 0;
}
