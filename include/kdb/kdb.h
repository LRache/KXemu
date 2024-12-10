#ifndef __KDB_H__
#define __KDB_H__

#include "cpu/cpu.h"
#include "memory/memory.h"

#include <string>

namespace kdb {
    void init();
    void deinit();

    void init_memory();
    void deinit_memory();

    void cmd_init();
    void run_cmd_mainloop();
    int run_command(const std::string &cmd);
    int run_cpu();

    extern Memory *memory;
    extern CPU *cpu;

    enum CmdErrorCode {
        CmdNotFound   = -1,
        Success       = 0,
        InvalidArgs   = 1,
        MissingPrevOp = 2,
    };
}

#endif
