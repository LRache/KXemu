#include <stdint.h>
#include "isa/riscv.h"

__attribute__((aligned(4 << 20))) uint8_t page[8][1 << 12];

void init_sv39() {
    word_t satp = 0;
    satp |= VM_SV39;
    satp |= ((word_t)(&page[2]) >> 12); // Set the root page table
    csrw_satp(satp);
    
    ((word_t *)page[2])[(0x20000000UL >> 30) & 0x1ff] = (((word_t)&page[3] >> 12) << 10) | PTE_V;
    ((word_t *)page[3])[(0x20000000UL >> 21) & 0x1ff] = (((word_t)&page[4] >> 12) << 10) | PTE_V;
    ((word_t *)page[4])[(0x20000000UL >> 12) & 0x1ff] = (((word_t)&page[0] >> 12) << 10) | PTE_V | PTE_U | PTE_X;
    ((word_t *)page[4])[(0x20001000UL >> 12) & 0x1ff] = (((word_t)&page[1] >> 12) << 10) | PTE_V | PTE_U | PTE_R | PTE_W;
    
    sfence_vma();
}

void init_sv48() {
    word_t satp = 0;
    satp |= VM_SV48;
    satp |= ((word_t)(&page[2]) >> 12); // Set the root page table
    csrw_satp(satp);
    
    ((word_t *)page[2])[(0x20000000UL >> 39) & 0x1ff] = (((word_t)&page[3] >> 12) << 10) | PTE_V;
    ((word_t *)page[3])[(0x20000000UL >> 30) & 0x1ff] = (((word_t)&page[4] >> 12) << 10) | PTE_V;
    ((word_t *)page[4])[(0x20000000UL >> 21) & 0x1ff] = (((word_t)&page[5] >> 12) << 10) | PTE_V;
    ((word_t *)page[5])[(0x20000000UL >> 12) & 0x1ff] = (((word_t)&page[0] >> 12) << 10) | PTE_V | PTE_U | PTE_X;
    ((word_t *)page[5])[(0x20001000UL >> 12) & 0x1ff] = (((word_t)&page[1] >> 12) << 10) | PTE_V | PTE_U | PTE_R | PTE_W;

    sfence_vma();
}

void init_sv57() {
    word_t satp = 0;
    satp |= VM_SV57;
    satp |= ((word_t)(&page[2]) >> 12);
    csrw_satp(satp);

    ((word_t *)page[2])[(0x20000000UL >> 48) & 0x1ff] = (((word_t)&page[3] >> 12) << 10) | PTE_V;
    ((word_t *)page[3])[(0x20000000UL >> 39) & 0x1ff] = (((word_t)&page[4] >> 12) << 10) | PTE_V;
    ((word_t *)page[4])[(0x20000000UL >> 30) & 0x1ff] = (((word_t)&page[5] >> 12) << 10) | PTE_V;
    ((word_t *)page[5])[(0x20000000UL >> 21) & 0x1ff] = (((word_t)&page[6] >> 12) << 10) | PTE_V;
    ((word_t *)page[6])[(0x20000000UL >> 12) & 0x1ff] = (((word_t)&page[0] >> 12) << 10) | PTE_V | PTE_U | PTE_X;
    ((word_t *)page[6])[(0x20001000UL >> 12) & 0x1ff] = (((word_t)&page[1] >> 12) << 10) | PTE_V | PTE_U | PTE_R | PTE_W;

    sfence_vma();
}

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
