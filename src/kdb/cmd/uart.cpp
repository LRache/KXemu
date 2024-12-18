#include "kdb/cmd.h"
#include "kdb/kdb.h"
#include "utils/utils.h"

#include <cstddef>
#include <iostream>

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
    bool s;
    word_t base = kdb::string_to_addr(args[2], s);
    if (!s) {
        std::cout << "Invalid address: " << args[2] << std::endl;
        return cmd::InvalidArgs;
    }
    if (args.size() == 3) {
        if (kdb::uart::add(base, std::cout)) {
            std::cout << "UART added at base " << std::hex << base << std::dec << std::endl;
            return cmd::Success;
        } else {
            return cmd::CmdError;
        }
    } else {
        int port = 0;
        try {
            port = std::stoi(args[3]);
        } catch (std::exception &) {
            std::cout << "Invalid port: " << args[3] << std::endl;
            return cmd::InvalidArgs;
        }
        if (kdb::uart::add(base, "localhost", port)) {
            std::cout << "UART added at base " << std::hex << base << std::dec << " port " << port << std::endl;
            return cmd::Success;
        } else {
            return cmd::CmdError;
        }
    }
}

static int cmd_uart_puts(const cmd::args_t &args) {
    if (args.size() < 3) {
        std::cout << "Usage: uart putch <base> <char>" << std::endl;
        return cmd::InvalidArgs;
    }
    
    bool s;
    std::size_t index = utils::string_to_word(args[2], s);
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
