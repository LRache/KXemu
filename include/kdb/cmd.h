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
    int log  (const std::vector<std::string> &);
    int mem  (const std::vector<std::string> &);
    int help (const std::vector<std::string> &);
    int quit (const std::vector<std::string> &);
    int reset(const std::vector<std::string> &);
    int step (const std::vector<std::string> &);

    // do command return code
    enum Code {
        EmptyArgs     = -2,
        CmdNotFound   = -1,
        Success       = 0,
        InvalidArgs   = 1,
        MissingPrevOp = 2,
        CmdError      = 3
    };

    extern Core *currentCore; // command line current cpu core
    extern int coreCount; // core count of cpu
}

#endif // __KDB_CMD_H__
