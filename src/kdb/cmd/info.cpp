#include "isa/isa.h"
#include "word.h"
#include "kdb/cmd.h"
#include "kdb/kdb.h"

#include <iostream>

using namespace kxemu;
using namespace kxemu::kdb;

static int cmd_info_gpr(const cmd::args_t &args);
static int cmd_info_pc(const cmd::args_t &args);

static const cmd::cmd_map_t cmdMap = {
    {"gpr", cmd_info_gpr},
    {"pc", cmd_info_pc},
};

static int cmd_info_gpr(const cmd::args_t &) {
    auto core = kdb::cpu->get_core(cmd::currentCore);
    for (unsigned int i = 0; i < isa::get_gpr_count(); i++) {
        word_t value = core->get_gpr(i);
        std::cout << isa::get_gpr_name(i) << " = " 
        << FMT_STREAM_WORD(value) 
        << "(" << value<< ")" << std::endl;
    }
    return cmd::Success;
}

static int cmd_info_pc(const cmd::args_t &) {
    auto core = kdb::cpu->get_core(cmd::currentCore);
    word_t pc = core->get_pc();
    std::cout << "pc = " << FMT_STREAM_WORD(pc) << std::endl;
    return cmd::Success;
}

int cmd::info(const args_t &args) {
    return cmd::find_and_run(args, cmdMap, 1);
}
