#include <stdint.h>

#define UART_BASE 0x10000000
#define UART_THR (*(volatile uint8_t *)(UART_BASE + 0))
#define UART_RBR (*(volatile uint8_t *)(UART_BASE + 0))
#define UART_LSR (*(volatile uint8_t *)(UART_BASE + 5))

inline int __am_uart_tx_ready() {
    return UART_LSR & (1 << 5);
}

void __am_uart_tx(uint8_t ch) {
    while (!__am_uart_tx_ready());
    UART_THR = ch;
}

inline int __am_uart_rx_ready() {
    return UART_LSR & (1 << 0);
}

uint8_t __uart_rx() {
    if (__am_uart_rx_ready()) {
        return UART_RBR;
    } else {
        return 0xff;
    }
}
