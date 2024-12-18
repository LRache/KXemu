#include <stdint.h>

#define UART_BASE 0x10000000
#define UART_LCR (*(volatile uint8_t *)(UART_BASE + 3))
#define UART_THR (*(volatile uint8_t *)(UART_BASE + 0))
#define UART_RBR (*(volatile uint8_t *)(UART_BASE + 0))
#define UART_FCR (*(volatile uint8_t *)(UART_BASE + 2))
#define UART_LSR (*(volatile uint8_t *)(UART_BASE + 5))

inline int __uart_tx_ready() {
    return UART_LSR & (1 << 5);
}

void __uart_tx(uint8_t ch) {
    while (!__uart_tx_ready());
    UART_THR = ch;
}

inline int __uart_rx_ready() {
    return UART_LSR & (1 << 0);
}

uint8_t __uart_rx() {
    if (__uart_rx_ready()) {
        return UART_RBR;
    } else {
        return 0xff;
    }
}

int main() {
    const char *str = "Hello, World! This is KXemu!\n";
    for (int i = 0; str[i]; i++) {
        __uart_tx(str[i]);
    }
    while (1) {
        uint8_t ch = __uart_rx();
        if (ch != 0xff) {
            __uart_tx(ch);
        }
    }
    return 0;
}
