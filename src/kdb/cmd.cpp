#include "isa/isa.h"
#include "kdb/kdb.h"
#include "log.h"

#include <map>
#include <readline/readline.h>
#include <readline/history.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#define SHELL_BULE "\x1b[36m"
#define SHELL_RESET "\x1b[0m"

static int coreCount = 1;
static int currentCore = 0;

static bool cmdRunning = false;

static std::vector<std::string> args;

typedef int (*cmd_func_t)();
std::map<std::string, cmd_func_t> cmdMap;

static int cmd_help() {
    std::cout << "Commands:" << std::endl;
    return 0;
}

static int cmd_quit() {
    std::cout << "Bye" << std::endl;
    cmdRunning = false;
    return 0;
}

void kdb::cmd_init() {
    if (kdb::cpu == nullptr || kdb::memory == nullptr) {
        PANIC("CPU or memory not initialized");
    }

    coreCount = cpu->coreCount();
    currentCore = 0;
    if (coreCount != 1) {
        WARN("Multiple cores detected, only core 0 is used");
    }

    cmdMap.insert(std::make_pair("help", cmd_help));
    cmdMap.insert(std::make_pair("h", cmd_help));
    cmdMap.insert(std::make_pair("quit", cmd_quit));
    cmdMap.insert(std::make_pair("q", cmd_quit));
}

void kdb::run_cmd() {
    cmdRunning = true;
    while (cmdRunning) {
        char *inputLine = readline(SHELL_BULE ISA_NAME "-kdb> " SHELL_RESET);
        if (inputLine == nullptr) {
            break;
        }
        
        args.clear();
        std::string cmd = inputLine;
        std::string arg;
        std::stringstream ss(cmd);
        while (std::getline(ss, arg, ' ')) {
            args.push_back(arg);
        }

        if (cmdMap.find(args[0]) != cmdMap.end()) {
            cmdMap[args[0]]();
            add_history(inputLine);
        } else {
            std::cout << "Unknown command: " << args[0] << std::endl;
        }

        free(inputLine);
    }
}
