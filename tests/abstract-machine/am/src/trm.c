#include "am.h"
#include <stdio.h>

#ifdef MAINARGS
    static const char *mainargs = "" MAINARGS "";
#else
    static const char *mainargs = "ref";
#endif

Area heap;

extern int main(const char *mainargs);

extern void __halt(int code);

__attribute__((noreturn))
void halt(int code) {
    __halt(code);
    while(1);
}

extern char __heap_start;
extern char __mem_end;

static void __trm_init() {
    heap.start = &__heap_start;
    heap.end = heap.start + 0x8000000;
}

void __run_main() {
    __trm_init();
    int r = main(mainargs);
    halt(r);
}

extern void __am_uart_tx(unsigned char);

int putchar(int c) {
    __am_uart_tx(c);
    return 0;
}
