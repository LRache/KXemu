#include "kdb/cmd.hpp"
#include "kdb/kdb.hpp"
#include "utils/utils.hpp"

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

static int cmd_set_halt_on_break(const cmd::args_t &args) {
    bool halt;
    if (args.size() == 2) {
        halt = kdb::cpu->get_core(cmd::currentCore)->get_halt_on_break();
    } else if (args.size() == 3) {
        if (args[2] == "on") {
            halt = true;
        } else if (args[2] == "off") {
            halt = false;
        } else {
            std::cerr << "Usage: set halt-on-break [on|off]" << std::endl;
            return cmd::InvalidArgs;
        }

        for (unsigned int i = 0; i < kdb::cpu->core_count(); i++) {
            kdb::cpu->get_core(i)->set_halt_on_break(halt);
        }
    } else {
        std::cerr << "Usage: set halt-on-break [on|off]" << std::endl;
        return cmd::InvalidArgs;
    }

    std::cout << "Halt on break instructions: " << (halt ? "on" : "off") << std::endl;
    return cmd::Success;
}

static cmd::cmd_map_t cmdMap = {
    {"reg", cmd_set_reg},
    {"halt-on-break", cmd_set_halt_on_break},
};

int cmd::set(const args_t &args) {
    if (args.size() < 2) {
        std::cerr << "Usage: set <type> ..." << std::endl;
        std::cerr << "       set reg <reg> <value>" << std::endl;
        std::cerr << "       set halt-on-break [on|off]" << std::endl;
        return cmd::EmptyArgs;
    }

    return cmd::find_and_run(args, cmdMap, 1);
}
