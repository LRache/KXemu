#ifndef __LOG_H__
#define __LOG_H__

#include <cstdio>

#define FMT_BLOD "m"

#define FMT_FG_RED     "\x1b[1;31m"
#define FMT_FG_GREEN   "\x1b[1;32m"
#define FMT_FG_YELLOW  "\x1b[1;33m"
#define FMT_FG_BLUE    "\x1b[1;34m"
#define FMT_FG_MAGENTA "\x1b[1;35m"
#define FMT_FG_CYAN    "\x1b[1;36m"
#define FMT_FG_RESET   "\x1b[1;0m"

#define PUTLOG(FG, TYPE, ...) \
do { \
    if (!(logFlag & TYPE)) break; \
    logTriggerFlag |= TYPE; \
    printf(FG "[" #TYPE "][%s:%d %s] " FMT_FG_RESET, __FILE__, __LINE__, __func__); \
    printf(__VA_ARGS__); \
    printf(FMT_FG_RESET "\n"); \
} while(0);

#define DEBUG(...) \
    PUTLOG(FMT_FG_YELLOW, DEBUG, __VA_ARGS__)

#define INFO(...) \
    PUTLOG(FMT_FG_BLUE, INFO, __VA_ARGS__)

#define WARN(...) \
    PUTLOG(FMT_FG_MAGENTA, WARN, __VA_ARGS__)

#define PANIC(...) \
do { \
    PUTLOG(FMT_FG_RED, PANIC, __VA_ARGS__); \
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
