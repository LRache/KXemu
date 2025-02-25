#ifndef __KXEMU_LOG_H__
#define __KXEMU_LOG_H__

#include "macro.h"
#include "config/autoconf.h"

#include <cstdio>

#define PUTLOG(FG, TYPE, ...) \
do { \
    if (!(kxemu::logFlag & kxemu::TYPE)) break; \
    printf(FG "[" #TYPE "][%s:%d %s] " FMT_FG_RESET, __FILE__, __LINE__, __func__); \
    printf(__VA_ARGS__); \
    printf(FMT_FG_RESET "\n"); \
} while(0);

#ifdef CONFIG_LOG
    #include "word.h"

    #define DEBUG(...) \
        PUTLOG(FMT_FG_YELLOW_BLOD, DEBUG, __VA_ARGS__) \

    #define INFO(...) \
        PUTLOG(FMT_FG_BLUE_BLOD, INFO, __VA_ARGS__)

    #define WARN(...) \
        PUTLOG(FMT_FG_MAGENTA_BLOD, WARN, __VA_ARGS__)

#else

    #define DEBUG(...)
    #define INFO(...)
    #define WARN(...)

#endif

#define PANIC(...) \
    do { \
        PUTLOG(FMT_FG_RED_BLOD, PANIC, __VA_ARGS__); \
        exit(1); \
    } while(0);

#ifdef CONFIG_HINT
#define HINT(...) \
    PUTLOG(FMT_FG_YELLOW_BLOD, HINT, __VA_ARGS__); 
#else
    #define HINT(...)
#endif

namespace kxemu {
    enum LogFlag {
    DEBUG = 1,
    INFO  = 2,
    WARN  = 4,
    PANIC = 8,
    HINT  = 16,
};

extern int logFlag;
} // namespace kxemu

#endif
