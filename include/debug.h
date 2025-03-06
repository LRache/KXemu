#ifndef __KXEMU_DEBUG_H__
#define __KXEMU_DEBUG_H__

#include "macro.h"
#include <cstdio>
#include <cstdlib>

#ifdef CONFIG_DEBUG

#define SELF_PROTECT(cond, ...) \
do { \
    if (unlikely(!(cond))) { \
        fprintf(stderr, FMT_FG_RED "[SELF-PROTECT][%s:%d %s]\nASSERT FAILED: %s\nThere must be wrong in your implemention. Please check.\n",\
         __FILE__, __LINE__, __func__, #cond); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, FMT_FG_RESET "\n"); \
        exit(1); \
    } \
} while(0);

#else

#define SELF_PROTECT(cond, ...) (void)cond;

#endif

#define NOT_IMPLEMENTED() \
do { \
    printf(FMT_FG_RED "[NOT-IMPLEMENTED][%s:%d %s]\nNot implemented yet.\n" FMT_FG_RESET, __FILE__, __LINE__, __func__); \
    exit(1); \
} while(0);

#endif
