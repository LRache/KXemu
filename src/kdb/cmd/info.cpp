#include "isa/isa.h"
#include "word.h"
#include "kdb/cmd.h"
#include "kdb/kdb.h"

#include <iostream>

using namespace kxemu;
using namespace kxemu::kdb;

static int cmd_info_gpr() {
    auto core = kdb::cpu->get_core(cmd::currentCore);
    for (unsigned int i = 0; i < isa::get_gpr_count(); i++) {
        word_t value = core->get_gpr(i);
        std::cout << isa::get_gpr_name(i) << " = " 
        << FMT_STREAM_WORD(value) 
        << "(" << value<< ")" << std::endl;
    }
    return cmd::Success;
}

static int cmd_info_reg(const std::string &name) {
    bool success;
    word_t v = kdb::cpu->get_core(cmd::currentCore)->get_register(name, success);
    
    if (!success) {
        std::cout << "Register not found: " << name << std::endl;
        return cmd::InvalidArgs;
    }

    std::cout << name <<" = " << FMT_STREAM_WORD(v) << std::endl;
    return cmd::Success;
}

int cmd::info(const args_t &args) {
    if (args.size() < 2) {
        std::cout << "Usage: info gpr/pc" << std::endl;
        return cmd::EmptyArgs;
    }

    if (args[1] == "gpr") {
        return cmd_info_gpr();
    } else {
        return cmd_info_reg(args[1]);
    }
}
