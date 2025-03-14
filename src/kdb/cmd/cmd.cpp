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
#include "utils/utils.h"

#include <readline/chardefs.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string>
#include <iostream>
#include <fstream>

#define SHELL_BULE "\x1b[94m"
#define SHELL_RESET "\x1b[0m"

using namespace kxemu;
using namespace kxemu::kdb;

unsigned int cmd::currentCore;
int cmd::coreCount;

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
    {"gdb"  , cmd::gdb},
    {"device", cmd::device}
};

static bool cmdRunning = true;

void cmd::init() {
    if (kdb::cpu == nullptr || kdb::bus == nullptr) {
        PANIC("CPU or memory not initialized");
    }

    cmd::coreCount = cpu->core_count();
    cmd::currentCore = 0;

    cmd::init_completion();
}

static const char *logo = \
" _  __ __  __                        \n"   
"| |/ / \\ \\/ /   ___   _ __ ___    _   _ \n"
"| ' /   \\  /   / _ \\ | '_ ` _ \\  | | | |\n"
"| . \\   /  \\  |  __/ | | | | | | | |_| |\n"
"|_|\\_\\ /_/\\_\\  \\___| |_| |_| |_|  \\__,_|\n";
                                        
int cmd::mainloop() {
    if (!cmdRunning) return kdb::returnCode;
    
    std::cout << SHELL_BULE << logo << SHELL_RESET << std::endl;
    
    char prompt[64];
    std::snprintf(prompt, sizeof(prompt), SHELL_BULE "%s-kdb> " SHELL_RESET, isa::get_isa_name());

    std::string lastCmd;
    char *inputLine;
    while (cmdRunning) {
        inputLine = readline(prompt);
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

int cmd::run_command(const std::string &cmd) {
    args_t args = utils::string_split(cmd, ' ');
    replace_define(args);
    int r = find_and_run(args, cmdMap);

    return r;
}

int cmd::run_source_file(const std::string &filename) {
    std::ifstream f;
    f.open(filename, std::ios::in);
    if (!f.is_open()) {
        std::cerr << "FileNotFound: No such file: " << filename << std::endl;
        return CmdError;
    }

    std::string cmdLine;
    while (std::getline(f, cmdLine)) {
        run_command(cmdLine);
    }
    return Success;
}

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

    return run_source_file(filename);
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
