/***************************************************************
 * Project Name: KXemu
 * File Name: include/kdb/cmd.h
 * Description: Define functions for kdb command line interface.
                Command line is just an interface for user to interact with the system and the KDB.
                Only do such things in the cmd::* functions:
                1. Parse the arguments from the user input.
                2. Open file stream and pass the stream to the corresponding functions.
                3. Call the corresponding functions to do the real work.
                4. Return the result code to the caller.
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
    void init_completion();

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
    int info    (const args_t &); // print info of cpu
    int uart    (const args_t &); // uart command
    int breakpoint(const args_t &); // set breakpoint

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
