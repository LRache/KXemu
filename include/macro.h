#ifndef __MACRO_H__
#define __MACRO_H__

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#endif
