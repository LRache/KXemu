#include "am.h"
#include "klib.h"
#include "isa/riscv.h"

#include <stdint.h>
#include <stdbool.h>

void __am_get_cur_as(Context *c);
void __am_switch(Context *c);

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
    return c;
}

extern void __am_asm_trap(void);

void cte_init(Context*(*handler)(Event, Context*)) {
    // initialize exception entry
    asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));
    // register event handler
    user_handler = handler;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
    Context *context = (Context *)(kstack.end - sizeof(Context));
    // context->mepc = (uintptr_t)entry - 4;
    // context->mstatus = 0x1800 | (1 << 3) | (1 << 7);
    // context->gpr[10] = (uintptr_t)arg;
    // context->gpr[2] = (uintptr_t)kstack.end;
    return context;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
    return false;
}

void iset(bool enable) {
}
