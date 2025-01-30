#include "klib.h"
#include "am.h"
#include "isa/riscv.h"
#include <stdio.h>
#include <string.h>

// page[0] -> user code
// page[1] -> user space for test
// page[2] -> fisrt stage page table
// page[3] -> second stage page table
__attribute__((aligned(4 << 20)))uint8_t page[4][4096];

// VM Mapping
// 0x20000000 ~ 0x20000fff -> page[0]
// 0x20001000 ~ 0x20001fff -> page[1]
static void init_vm() {
    word_t satp = 0;
    satp |= (1 << 31); // Sv32 mode
    satp |= ((word_t)(&page[2]) >> 12); // Set the root page table
    csrw_satp(satp);

    // Set fisrt stage page table
    ((word_t *)page[2])[(0x20000000 >> 22) & 0x3ff] = (((word_t)&page[3] >> 12) << 10) | PTE_V;
    // Map page[0] to 0x20000000
    ((word_t *)page[3])[(0x20000000 >> 12) & 0x3ff] = (((word_t)&page[0] >> 12) << 10) | PTE_V | PTE_U | PTE_X;
    // Map page[1] to 0x20001000
    ((word_t *)page[3])[(0x20001000 >> 12) & 0x3ff] = (((word_t)&page[1] >> 12) << 10) | PTE_V | PTE_U | PTE_R | PTE_W;

    sfence_vma();
}

static void init_vm_superpage() {
    word_t satp = 0;
    satp |= (1 << 31); // Sv32 mode
    satp |= ((word_t)(&page[2]) >> 12); // Set the root page table
    csrw_satp(satp);

    // Set fisrt stage page table
    ((word_t *)page[2])[(0x20000000 >> 22) & 0x3ff] = (((word_t)&page[0] >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X | PTE_U;

    sfence_vma();
}

// User code
extern char user_img_start[];
extern char user_img_end[];

void load_user_img() {
    char *src = user_img_start;
    char *dst = (char *)page[0];
    while (src < user_img_end) {
        *dst = *src;
        src++;
        dst++;
    }
}


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
    if (count == 1 || count == 3) {
        if (context->gpr[6] != 0x10ADDA7A) {
            printf("Test%d FAIL\n", count);
            halt(2);
        }
    } else if (count == 2 || count == 4) {
        if (*(uint32_t *)(page[1] + 0xa + 0x4) != 0x52CAFFEE) {
            printf("Test%d FAIL\n", count);
            halt(3);
        }
    }
    printf("Test%d PASS\n", count);

    if (count == 2) {
        init_vm_superpage();
        memset(page[1], 0, sizeof(page[1]));
        *(uint32_t *)(page[1] + 0xa) = 0x10ADDA7A; // load data
    }

    if (count == 4) {
        printf("All test PASS\n");
        halt(0);
    }
    
    uintptr_t mepc = csrr_mepc();
    csrw_mepc(mepc + 4);

    return context;
}

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
    init_vm();

    // set the mtvec to the address of the trap handler
    csrw_mtvec((word_t)m_trap_entry & ~0b11);
    csrw_mepc(0x20000000);

    Context context = {};
    context.gpr[2] = 0x20002000;

    *(uint32_t *)(page[1] + 0xa) = 0x10ADDA7A; // load data
    
    user_entry(&context);

    return 0;
}
