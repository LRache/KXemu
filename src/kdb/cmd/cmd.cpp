#include "cpu/cpu.h"
#include "isa/isa.h"
#include "kdb/kdb.h"
#include "log.h"
#include "utils/disasm.h"

#include <map>
#include <readline/readline.h>
#include <readline/history.h>
#include <string>
#include <utility>
#include <vector>
#include <sstream>
#include <iostream>

#define SHELL_BULE "\x1b[36m"
#define SHELL_RESET "\x1b[0m"

static int coreCount = 1;
static int currentCore = 0;

static bool cmdRunning = false;

typedef int (*cmd_func_t)(const std::vector<std::string> &);
std::map<std::string, cmd_func_t> cmdMap;

static int cmd_help(const std::vector<std::string> &) {
    std::cout << "Commands:" << std::endl;
    return 0;
}

static int cmd_quit(const std::vector<std::string> &) {
    std::cout << "Bye~" << std::endl;
    cmdRunning = false;
    return 0;
}

static int cmd_step(const std::vector<std::string> &args) {
    std::string ns = args[1]; // step count
    unsigned long n = std::stoul(ns);

    Core *core = kdb::cpu->get_core(currentCore);
    for (unsigned long i = 0; i < n; i++) {
        if (core->is_break() || core->is_error()) {
            break;
        }
    }
    return 0;
}

void kdb::cmd_init() {
    if (kdb::cpu == nullptr || kdb::memory == nullptr) {
        PANIC("CPU or memory not initialized");
    }

    coreCount = cpu->core_count();
    currentCore = 0;
    if (coreCount != 1) {
        WARN("Multiple cores detected, only core 0 is used");
    }

    cmdMap.insert(std::make_pair("help", cmd_help));
    cmdMap.insert(std::make_pair("h", cmd_help));
    cmdMap.insert(std::make_pair("quit", cmd_quit));
    cmdMap.insert(std::make_pair("q", cmd_quit));
    cmdMap.insert(std::make_pair("exit", cmd_quit));
    cmdMap.insert(std::make_pair("step", cmd_step));
    cmdMap.insert(std::make_pair("s", cmd_step));

    disasm::init("riscv32");
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
        
        
        int r = run_command(cmd);
        if (r == -1) {
            std::string cmdName = cmd.substr(0, cmd.find(' '));
            std::cout << "Unknown command: " << cmdName << std::endl;
        }

        free(inputLine);
    }
}

int kdb::run_command(std::string cmd) {
    std::vector<std::string> args;
    std::string arg;
    std::stringstream ss(cmd);
    while (std::getline(ss, arg, ' ')) {
        args.push_back(arg);
    }

    if (cmdMap.find(args[0]) != cmdMap.end()) {
        return cmdMap[args[0]](args);
    }
    return -1;
}
