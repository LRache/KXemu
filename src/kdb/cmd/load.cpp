#include "kdb/kdb.h"
#include "kdb/cmd.h"
#include "utils/utils.h"
#include "word.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <fstream>
#include <optional>

using namespace kxemu;
using namespace kxemu::kdb;

static int cmd_load_elf(const cmd::args_t &);
static int cmd_load_bin(const cmd::args_t &);
static int cmd_load_symbol(const cmd::args_t &);

static const cmd::cmd_map_t cmdMap = {
    {"elf", cmd_load_elf},
    {"bin", cmd_load_bin},
    {"symbol", cmd_load_symbol}
};

static int cmd_load_elf(const cmd::args_t &args) {
    if (args.size() <= 2) {
        std::cout << "Usage: load elf <filename>" << std::endl;
        return cmd::EmptyArgs;
    }
    
    auto entry = kdb::load_elf(args[2]);
    if (entry == std::nullopt) {
        std::cerr << "Failed to load ELF file " << args[2] << std::endl;
        return cmd::CmdError;
    }
    
    std::cout << "Load ELF file success." << std::endl;
    std::cout << "Switch entry to " << FMT_STREAM_WORD(entry.value()) << "." << std::endl;
    kdb::programEntry = entry.value();
    
    return cmd::Success;
}

static int cmd_load_bin(const cmd::args_t &args) {
    if (args.size() < 4) {
        std::cout << "Usage: load bin <filename> <addr>" << std::endl;
        return cmd::EmptyArgs;
    }
    const std::string &filename = args[2];
    auto addr = utils::string_to_unsigned(args[3]);
    if (!addr.has_value()) {
        std::cout << "Invalid address: " << args[3] << std::endl;
        return cmd::InvalidArgs;
    }

    std::ifstream f;
    f.open(filename, std::ios::in | std::ios::binary);
    if (!f.is_open()) {
        std::cout << "Open file \"" << filename << "\" FAILED: " << std::strerror(errno) << std::endl;
        return cmd::CmdError;
    }

    if (kdb::bus->load_from_stream(f, addr.value())) {
        std::cout << "Load binary file success." << std::endl;
        return cmd::Success;
    } else {
        std::cerr << "Failed to load binary file " << filename << std::endl;
        return cmd::CmdError;
    }
}

static int cmd_load_symbol(const cmd::args_t &args) {
    if (args.size() <= 2) {
        std::cout << "Usage: load symbol <filename>" << std::endl;
        return cmd::EmptyArgs;
    }

    const std::string &filename = args[2];
    if (kdb::load_symbol(filename)) {
        return cmd::Success; 
    } else {
        return cmd::CmdError;
    }
}

int cmd::load(const cmd::args_t & args) {
    int r = cmd::find_and_run(args, cmdMap, 1);
    return r;
}
