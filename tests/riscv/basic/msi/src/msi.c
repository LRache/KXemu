#include "am.h"
#include "klib.h"
#include "isa/riscv.h"

#include <stdint.h>

#define ACLINT_BASE 0x02000000
#define ACLINT_MSIP_BASE (ACLINT_BASE + 0)
#define MSIP(i) *((volatile uint32_t *)(ACLINT_MSIP_BASE) + i)

__attribute__((aligned(4)))
void handler() {
    uintptr_t cause = csrr_mcause();
    printf("mcause=%llx\n", cause);
    halt(0);
}

static volatile int lock = 0;

int main() {
    printf("Hello World!\n");

    csrw_mtvec((uintptr_t)handler);
    unsigned int hartid = csrr_mhartid();
    if (hartid == 1) {
        MSIP(0) = 1;
        lock = 1;
    } else {
        while(!lock);
        uintptr_t mstatus = csrr_mstatus();
        mstatus |= (1 << 3);
        csrw_mstatus(mstatus);
        
        uintptr_t mie = csrr_mie();
        mie |= (1 << 3);
        csrw_mie(mie);

        wfi();
    }
    return 0;
}