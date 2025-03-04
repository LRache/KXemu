#include "isa/riscv.h"
#include "am-dev.h"
#include "klib.h"

#ifdef __QEMU
#endif

#define UART_BASE 0x10000000
#define UART_IER (*(volatile uint8_t *)(UART_BASE + 1))

#define PLIC_BASE 0x0C000000
#define PLIC_R(x) (*(volatile uint32_t *)(PLIC_BASE + (x)))

Context *handler(Context *context) {
    uint8_t c = __uart_rx();
    __am_uart_tx(c);
    
    PLIC_R(0x200004) = 10;

    return context;
}

int main() {
    cte_init(handler);
    
    word_t mstatus = csrr_mstatus();
    mstatus |= MSTATUS_MIE_MASK;
    csrw_mstatus(mstatus);

    word_t mie = csrr_mie();
    mie |= MIE_MEIE_MASK;
    csrw_mie(mie);

    UART_IER = 0x1;
    PLIC_R(4 * 10) = 1;
    PLIC_R(0x2000) = 1 << 10;
    PLIC_R(0x200000) = 0;

    printf("Hello, World! This is KXemu!\n");
    
    while (1) {
        wfi();
    }
    
    return 0;
}
