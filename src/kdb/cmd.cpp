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
#include "utils/utils.h"

#include <readline/readline.h>
#include <readline/history.h>
#include <string>
#include <iostream>

#define SHELL_BULE "\x1b[94m"
#define SHELL_RESET "\x1b[0m"

using namespace kxemu;
using namespace kxemu::kdb;
using kxemu::cpu::Core;

static const cmd::cmd_map_t cmdMap = {
    {"help" , cmd::help  },
    {"h"    , cmd::help  },
    {"quit" , cmd::quit  },
    {"q"    , cmd::quit  },
    {"exit" , cmd::quit  },
    {"step" , cmd::step  },
    {"s"    , cmd::step  },
    {"run"  , cmd::run   },
    {"r"    , cmd::run   },
    {"source", cmd::source},
    {"reset", cmd::reset },
    {"log"  , cmd::log   },
    {"mem"  , cmd::mem   },
    {"symbol", cmd::symbol},
    {"sym"  , cmd::symbol},
    {"load" , cmd::load  },
    {"info" , cmd::info },
    {"break", cmd::breakpoint},
    {"x"    , cmd::show_mem},
    {"uart" , cmd::uart}
};

static bool cmdRunning = true;

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

int cmd::source(const args_t &args) {
    if (args.size() < 2) {
        std::cout << "Usage: source <filename>";
        return cmd::EmptyArgs;
    }
    std::string filename = args[1];
    std::cout << "Run source " << filename << std::endl;
    return kdb::run_source_file(filename);
}

int cmd::find_and_run(const args_t &args, const cmd_map_t &cmdMap, const std::size_t startIndex) {
    if (args.size() <= startIndex) {
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

static const char *logo = \
"  _  __  ____    ____  \n" \
" | |/ / |  _ \\  | __ ) \n" \
" | ' /  | | | | |  _ \\ \n" \
" | . \\  | |_| | | |_) | \n" \
" |_|\\_\\ |____/  |____/ \n";

void kdb::cmd_init() {
    if (kdb::cpu == nullptr || kdb::bus == nullptr) {
        PANIC("CPU or memory not initialized");
    }

    cmd::coreCount = cpu->core_count();
    cmd::currentCore = cpu->get_core(0);
    if (cmd::coreCount != 1) {
        WARN("Multiple cores detected, only core 0 is used");
    }

    disasm::init(ISA_NAME);
    cmd::init_completion();

    std::cout << SHELL_BULE << logo << SHELL_RESET << std::endl;
}

int kdb::run_cmd_mainloop() {
    std::string lastCmd;
    while (cmdRunning) {
        char *inputLine = readline(SHELL_BULE ISA_NAME "-kdb> " SHELL_RESET);
        if (inputLine == nullptr) {
            break;
        }

        std::string cmd = inputLine;
        free(inputLine);
        if (cmd.empty()) {
            
            if (!lastCmd.empty()) {
                run_command(lastCmd);
            }
            continue;
        }
        add_history(cmd.c_str());
        
        run_command(cmd);
        lastCmd = cmd;
    }

    return kdb::returnCode;
}

int kdb::run_command(const std::string &cmd) {
    cmd::args_t args = utils::string_split(cmd, ' ');
    int r = cmd::find_and_run(args, cmdMap);

    return r;
}
