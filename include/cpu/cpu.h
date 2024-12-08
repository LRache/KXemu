#ifndef __CPU_H__
#define __CPU_H__

#include "memory/memory.h"
#include "common.h"

class Core {
public:
    virtual bool is_error() = 0;
    virtual bool is_break() = 0;
    virtual ~Core() = default;
    word_t trapCode;
    word_t trapPC;
    word_t haltPC;
};

class CPU {
public:
    // flags for extension features
    virtual void init(Memory *memory, int flags, int coreCount) = 0;
    virtual void reset() = 0;
    virtual void step() = 0;
    virtual bool has_break() = 0;

    virtual int coreCount() = 0;
    virtual Core *getCore(int coreID) = 0;

    virtual ~CPU() {};
};

#endif
