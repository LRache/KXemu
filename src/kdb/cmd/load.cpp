#include "kdb/kdb.h"
#include "kdb/cmd.h"
#include "word.h"

#include <iostream>
#include <optional>

using namespace kxemu;
using namespace kxemu::kdb;

std::string cmd::elfFileName;

static int cmd_load_elf(const cmd::args_t &);

static const cmd::cmd_map_t cmdMap = {
    {"elf", cmd_load_elf},
};

static int cmd_load_elf(const cmd::args_t &args) {
    if (args.size() <= 2) {
        std::cout << "Usage: load elf <filename>" << std::endl;
        return cmd::EmptyArgs;
    }
    
    auto entry = kdb::load_elf(args[2]);
    if (entry == std::nullopt) {
        std::cerr << "Failed to load ELF file " << cmd::elfFileName << std::endl;
        return cmd::CmdError;
    }
    
    std::cout << "Load ELF file success." << std::endl;
    std::cout << "Switch entry to " << FMT_STREAM_WORD(entry.value()) << "." << std::endl;
    kdb::programEntry = entry.value();
    
    return cmd::Success;
}

int cmd::load(const cmd::args_t & args) {
    int r = cmd::find_and_run(args, cmdMap, 1);
    if (r == cmd::EmptyArgs) {
        std::cout << "Usage: load elf" << std::endl;
    }
    return r;
}
