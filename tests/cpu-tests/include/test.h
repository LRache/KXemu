#ifndef __TRAP_H__
#define __TRAP_H__

#include <am.h>
#include <klib.h>
#include <stdbool.h>

#define LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

__attribute__((noinline))
void check(bool cond) {
  if (!cond) halt(1);
}

#endif
