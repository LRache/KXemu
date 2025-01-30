#ifndef __KXEMU_MACRO_H__
#define __KXEMU_MACRO_H__

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#define MACRO_TO_STRING(x) #x

#define ARRLEN(array) (sizeof(array) / sizeof(array[0]))

#define FMT_FG_RED     "\x1b[31m"
#define FMT_FG_GREEN   "\x1b[32m"
#define FMT_FG_YELLOW  "\x1b[33m"
#define FMT_FG_BLUE    "\x1b[34m"
#define FMT_FG_MAGENTA "\x1b[35m"
#define FMT_FG_CYAN    "\x1b[36m"

#define FMT_FG_RED_BLOD     "\x1b[1;31m"
#define FMT_FG_GREEN_BLOD   "\x1b[1;32m"
#define FMT_FG_YELLOW_BLOD  "\x1b[1;33m"
#define FMT_FG_BLUE_BLOD    "\x1b[1;34m"
#define FMT_FG_MAGENTA_BLOD "\x1b[1;35m"
#define FMT_FG_CYAN_BLOD    "\x1b[1;36m"

#define FMT_FG_RESET   "\x1b[0m"

#endif
