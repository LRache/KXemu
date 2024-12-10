/***************************************************************
 * Project Name: KXemu
 * File Name: src/kdb/cmd/cmd.cpp
 * Description: Implement functions for kdb command line interface.
 ***************************************************************/

#include "cpu/cpu.h"
#include "isa/isa.h"
#include "kdb/kdb.h"
#include "kdb/cmd.h"
#include "log.h"
#include "utils/disasm.h"

#include <cstddef>
#include <cstdint>
#include <ios>
#include <map>
#include <readline/readline.h>
#include <readline/history.h>
#include <string>
#include <utility>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>

#define SHELL_BULE "\x1b[36m"
#define SHELL_RESET "\x1b[0m"

std::map<std::string, cmd::func_t> cmdMap = {
    {"help" , cmd::help  },
    {"h"    , cmd::help  },
    {"quit" , cmd::quit  },
    {"q"    , cmd::quit  },
    {"exit" , cmd::quit  },
    {"step" , cmd::step  },
    {"s"    , cmd::step  },
    {"reset", cmd::reset },
    {"log"  , cmd::log   },
    {"mem"  , cmd::mem   },
};

static bool cmdRunning = false;

Core *cmd::currentCore;
int cmd::coreCount;

int cmd::help(const args_t &) {
    std::cout << "Commands:" << std::endl;
    return 0;
}

int cmd::quit(const args_t &) {
    std::cout << "Bye~" << std::endl;
    cmdRunning = false;
    return 0;
}

int cmd::find_and_run(const args_t &args, const cmd_map_t &cmdMap, const std::size_t startIndex) {
    if (args.size() < 1) {
        return cmd::EmptyArgs;
    }

    auto iter = cmdMap.find(args[startIndex]);
    if (iter != cmdMap.end()) {
        return iter->second(args);
    }
    
    // NOTE: We only output command not found message in this function to avoid the recursion handle.
    if (startIndex == 0) {
        std::cout << "Command not found: " << args[startIndex] << std::endl;
    } else {
        std::cout << "Sub command not found: " << args[startIndex] << std::endl;
    }
    
    return cmd::CmdNotFound;
}

void kdb::cmd_init() {
    if (kdb::cpu == nullptr || kdb::memory == nullptr) {
        PANIC("CPU or memory not initialized");
    }

    cmd::coreCount = cpu->core_count();
    cmd::currentCore = cpu->get_core(0);
    if (cmd::coreCount != 1) {
        WARN("Multiple cores detected, only core 0 is used");
    }

    disasm::init(ISA_NAME);
}

void kdb::run_cmd_mainloop() {
    cmdRunning = true;
    while (cmdRunning) {
        char *inputLine = readline(SHELL_BULE ISA_NAME "-kdb> " SHELL_RESET);
        if (inputLine == nullptr) {
            break;
        }

        std::string cmd = inputLine;
        if (cmd.empty()) continue;
        add_history(inputLine);
        
        run_command(cmd);

        free(inputLine);
    }
}

int kdb::run_command(const std::string &cmd) {
    cmd::args_t args;
    std::string arg;
    std::stringstream ss(cmd);
    while (std::getline(ss, arg, ' ')) {
        args.push_back(arg);
    }

    int r = cmd::find_and_run(args, cmdMap);

    return r;
}
