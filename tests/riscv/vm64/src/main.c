#include "isa/riscv.h"
#include "klib.h"

extern uint8_t page[8][1 << 12];
void init_sv39();
void init_sv48();
void init_sv57();

// Trap handler
extern void m_trap_entry();
extern void user_entry(Context *context);

static int count = 0;
Context *m_trap_handler(Context *context) {
    uintptr_t mcause = csrr_mcause();

    printf("mcause = %d\n", mcause);
    if (mcause != 8) {
        halt(1);
    }
    
    count ++;
    if (count % 2 == 1) {
        if (context->gpr[6] != 0x10ADDA7A) {
            printf("Test%d FAIL\n", count);
            halt(2);
        }
    } else {
        if (*(word_t *)(page[1] + 0xa + 0x8) != 0x52CAFFEE) {
            printf("Test%d FAIL\n", count);
            halt(3);
        }
    }
    printf("Test%d PASS\n", count);

    if (count == 2) {
        init_sv48();
        memset(page[1], 0, sizeof(page[1]));
        *(uint32_t *)(page[1] + 0xa) = 0x10ADDA7A;
    } else if (count == 4) {
        init_sv57();
        memset(page[1], 0, sizeof(page[1]));
        *(uint32_t *)(page[1] + 0xa) = 0x10ADDA7A;
    } else if (count == 6) {
        printf("All test PASS\n");
        halt(0);
    }
    
    uintptr_t mepc = csrr_mepc();
    csrw_mepc(mepc + 4);

    return context;
}

void load_user_img();

int main() {
    // Let umode access the memory
    csrw_pmpcfg0(0xf);
    csrw_pmpaddr0(0xffffffff);

    // set the mstatues.mpp to U mode
    word_t mstatus = csrr_mstatus();
    mstatus &= ~MSTATUS_MPP_MASK;
    mstatus |= MSTATUS_MPP_U;
    csrw_mstatus(mstatus);

    // Load user code
    load_user_img();

    // Init VM
    init_sv39();

    // set the mtvec to the address of the trap handler
    csrw_mtvec((word_t)m_trap_entry & ~0b11);
    csrw_mepc(0x20000000);

    Context context = {};
    context.gpr[2] = 0x20002000;

    *(uint32_t *)(page[1] + 0xa) = 0x10ADDA7A; // load data
    
    user_entry(&context);

    return 0;
}