#ifndef __AM_H__
#define __AM_H__

#include "isa/riscv.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    void *start;
    void *end;
} Area;

extern Area heap;

#ifdef __cplusplus
extern "C" {
#endif

// TRM
void halt(int code) __attribute__((noreturn));
int  putchar(int c);

#ifdef __cplusplus
};
#endif

#endif
