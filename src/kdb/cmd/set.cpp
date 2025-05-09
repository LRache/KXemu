#include "kdb/cmd.h"
#include "kdb/kdb.h"
#include "utils/utils.h"

#include <iostream>

using namespace kxemu;
using namespace kxemu::kdb;

static int cmd_set_reg(const cmd::args_t &args) {
    if (args.size() != 4) {
        std::cerr << "Usage: set reg <reg> <value>" << std::endl;
        return cmd::InvalidArgs;
    }
    
    auto core = kdb::cpu->get_core(cmd::currentCore);
    bool success;
    word_t value = utils::string_to_unsigned(args[3], success);
    if (!success) {
        std::cerr << "Invalid value: " << args[3] << std::endl;
        return cmd::InvalidArgs;
    }
    
    core->set_register(args[2], value);
    return cmd::Success;
}

static cmd::cmd_map_t cmdMap = {
    {"reg", cmd_set_reg},
};

int cmd::set(const args_t &args) {
    if (args.size() < 4) {
        std::cerr << "Usage: set <type> <reg> <value>" << std::endl;
        return cmd::EmptyArgs;
    }

    return cmd::find_and_run(args, cmdMap, 1);
}
