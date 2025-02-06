#include "am.h"
#include "klib-macros.h"

Context *handler(Event ev, Context *ctx) {
    switch(ev.event) {
        case EVENT_IRQ_TIMER: putchar('t'); break;
        case EVENT_IRQ_IODEV: putchar('d'); break;
        case EVENT_SYSCALL:   putchar('s'); break;
        default: putstr("Unknown event\n");
    }
    ctx->mepc += 4;
    return ctx;
}

int main() {
    cte_init(handler);
    while (1) {
        yield();
        for (volatile int i = 0; i < 1000000; i++);
    }
    return 0;
}
