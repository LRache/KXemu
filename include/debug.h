#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "log.h"

#define SELF_PROTECT(cond, ...) \
do { \
    if (unlikely(!(cond))) { \
        printf(FMT_FG_RED "[SELF-PROTECT][%s:%d %s]\nSELF_PROTECT failed: %s\nThere must be wrong in your implemention. Please check.\n" FMT_FG_RESET,\
         __FILE__, __LINE__, __func__, #cond); \
        printf(__VA_ARGS__); \
        printf(FMT_FG_RESET "\n"); \
        exit(1); \
    } \
} while(0);

#define NOT_IMPLEMENTED() \
do { \
    printf(FMT_FG_RED "[NOT_IMPLEMENTED][%s:%d %s]\nNot implemented yet.\n" FMT_FG_RESET, __FILE__, __LINE__, __func__); \
    exit(1); \
} while(0);

#endif
