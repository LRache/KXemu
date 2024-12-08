#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <cstdio>

#define FMT_COLOR_RED     "\x1b[1;31m"
#define FMT_COLOR_GREEN   "\x1b[1;32m"
#define FMT_COLOR_YELLOW  "\x1b[1;33m"
#define FMT_COLOR_BLUE    "\x1b[1;34m"
#define FMT_COLOR_MAGENTA "\x1b[1;35m"
#define FMT_COLOR_CYAN    "\x1b[1;36m"
#define FMT_COLOR_RESET   "\x1b[1;0m"

#define PUTLOG(COLOR, TYPE, ...) \
do { \
    if (!(logFlag & TYPE)) break; \
    printf(COLOR "[" #TYPE "][%s:%d %s] " FMT_COLOR_RESET, __FILE__, __LINE__, __func__); \
    printf(__VA_ARGS__); \
    printf(FMT_COLOR_RESET "\n"); \
} while(0);

#define DEBUG(...) \
    PUTLOG(FMT_COLOR_YELLOW, DEBUG, __VA_ARGS__)

#define INFO(...) \
    PUTLOG(FMT_COLOR_BLUE, INFO, __VA_ARGS__)

#define WARN(...) \
    PUTLOG(FMT_COLOR_MAGENTA, WARN, __VA_ARGS__)

#define PANIC(...) \
do { \
    PUTLOG(FMT_COLOR_RED, PANIC, __VA_ARGS__); \
    exit(1); \
} while(0);

enum LogFlag {
    DEBUG = 1,
    INFO  = 2,
    WARN  = 4,
    PANIC = 8,
};

extern int logFlag;

#endif
