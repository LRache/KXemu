/***************************************************************
 * Project Name: KXemu
 * File Name: include/kdb/cmd.h
 * Description: Define functions for kdb command line interface.
 ***************************************************************/

#ifndef __KDB_CMD_H__
#define __KDB_CMD_H__

#include "cpu/cpu.h"
#include <cstddef>
#include <vector>
#include <map>
#include <string>

namespace cmd {
    using args_t = std::vector<std::string>;
    using func_t = int (*)(const args_t &);
    using cmd_map_t = std::map<std::string, func_t>;
    int find_and_run(const args_t &args, const cmd_map_t &cmdMap, std::size_t startArgs = 0);

    // do command function
    int log     (const args_t &);
    int mem     (const args_t &);
    int help    (const args_t &);
    int quit    (const args_t &);
    int source  (const args_t &);
    int reset   (const args_t &);
    int step    (const args_t &);
    int run     (const args_t &);
    int symbol  (const args_t &); // print symbol table from ELF
    int load    (const args_t &); // load image from filename given by args

    // do command return code
    enum Code {
        EmptyArgs     = -2,
        CmdNotFound   = -1,
        Success       = 0,
        InvalidArgs   = 1,
        MissingPrevOp = 2,
        CmdError      = 3
    };

    // load source from args
    extern std::string elfFileName;

    extern Core *currentCore; // command line current cpu core
    extern int coreCount; // core count of cpu
} // cmd

#endif // __KDB_CMD_H__
