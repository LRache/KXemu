#include "kdb/cmd.h"
#include "kdb/kdb.h"
#include "utils/utils.h"

#include <cstddef>
#include <iostream>

using namespace kxemu;
using namespace kxemu::kdb;

static int cmd_uart_puts(const cmd::args_t &);

static const cmd::cmd_map_t cmdMap = {
    {"puts", cmd_uart_puts},
};

static int cmd_uart_puts(const cmd::args_t &args) {
    if (args.size() < 3) {
        std::cout << "Usage: uart putch <base> <char>" << std::endl;
        return cmd::InvalidArgs;
    }
    
    bool s;
    std::size_t index = utils::string_to_unsigned(args[2], s);
    if (!s) {
        std::cout << "Invalid Index: " << args[2] << std::endl;
        return cmd::InvalidArgs;
    }
    
    if (kdb::uart::puts(index, args[3])) {
        return cmd::Success;
    } else {
        std::cout << "Invalid Index: " << args[2] << std::endl;
        return cmd::CmdError;
    }
}

int cmd::uart(const args_t &args) {
    return cmd::find_and_run(args, cmdMap, 1);
}
