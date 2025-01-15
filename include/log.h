#ifndef __KXEMU_LOG_H__
#define __KXEMU_LOG_H__

#include "macro.h"

#include <cstdio>

#define PUTLOG(FG, TYPE, ...) \
do { \
    if (!(logFlag & TYPE)) break; \
    logTriggerFlag |= TYPE; \
    printf(FG "[" #TYPE "][%s:%d %s] " FMT_FG_RESET, __FILE__, __LINE__, __func__); \
    printf(__VA_ARGS__); \
    printf(FMT_FG_RESET "\n"); \
} while(0);

#define DEBUG(...) \
    PUTLOG(FMT_FG_YELLOW_BLOD, DEBUG, __VA_ARGS__)

#define INFO(...) \
    PUTLOG(FMT_FG_BLUE_BLOD, INFO, __VA_ARGS__)

#define WARN(...) \
    PUTLOG(FMT_FG_MAGENTA_BLOD, WARN, __VA_ARGS__)

#define PANIC(...) \
do { \
    PUTLOG(FMT_FG_RED_BLOD, PANIC, __VA_ARGS__); \
    exit(1); \
} while(0);

enum LogFlag {
    DEBUG = 1,
    INFO  = 2,
    WARN  = 4,
    PANIC = 8,
};

extern int logFlag;
extern int logTriggerFlag;

#endif
