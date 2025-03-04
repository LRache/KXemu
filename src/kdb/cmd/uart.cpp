#include "word.h"
#include "kdb/cmd.h"
#include "kdb/kdb.h"
#include "utils/utils.h"

#include <cstddef>
#include <iostream>

using namespace kxemu;
using namespace kxemu::kdb;

static int cmd_uart_add (const cmd::args_t &);
static int cmd_uart_puts(const cmd::args_t &);

static const cmd::cmd_map_t cmdMap = {
    {"add", cmd_uart_add},
    {"puts", cmd_uart_puts},
};

static int cmd_uart_add(const cmd::args_t &args) {
    if (args.size() < 3) {
        std::cout << "Usage: uart add <base> [port]" << std::endl;
        return cmd::InvalidArgs;
    }
    
    // bool s;
    // word_t base = utils::string_to_unsigned(args[2], s);
    // if (!s) {
    //     std::cout << "Invalid address: " << args[2] << std::endl;
    //     return cmd::InvalidArgs;
    // }
    
    // if (args.size() == 3) {
    //     if (kdb::uart::add(base, std::cout)) {
    //         std::cout << "UART added at base " << std::hex << base << std::dec << std::endl;
    //         return cmd::Success;
    //     } else {
    //         return cmd::CmdError;
    //     }
    // } else {
    //     bool s;
    //     int port = utils::string_to_unsigned(args[3], s);
    //     if (!s) {
    //         std::cout << "Invalid port: " << args[3] << std::endl;
    //         return cmd::InvalidArgs;
    //     }

    //     if (kdb::uart::add(base, "127.0.0.1", port)) {
    //         std::cout << "UART added at base " << FMT_STREAM_WORD(base) << " port " << port << std::endl;
    //         return cmd::Success;
    //     } else {
    //         std::cout << "Failed to add UART." << std::endl;
    //         return cmd::CmdError;
    //     }
    // }
    return cmd::CmdError;
}

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
