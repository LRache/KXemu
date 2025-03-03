#include "am-dev.h"
#include "klib.h"

int main() {
    printf("Hello, World! This is KXemu!\n");
    while (1) {
        uint8_t c = __uart_rx();
        while (c == 0xff) c = __uart_rx();
        __am_uart_tx(c);
    }
    return 0;
}
