#ifndef __AM_H__
#define __AM_H__

#include "isa/riscv.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    void *start;
    void *end;
} Area;

typedef struct {
    enum {
        EVENT_NULL = 0,
        EVENT_SYSCALL, EVENT_PAGEFAULT, EVENT_ERROR, EVENT_IRQ_TIMER, EVENT_IRQ_IODEV,
    } event;
    uintptr_t cause, ref;
    const char *msg;
} Event;


extern Area heap;

#ifdef __cplusplus
extern "C" {
#endif

// TRM
void halt(int code) __attribute__((noreturn));
int  putchar(int c);

// CTE
bool cte_init(Context*(*handler)(Event, Context*));
void yield();

#ifdef __cplusplus
};
#endif

#endif
