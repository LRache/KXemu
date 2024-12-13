#include "isa/isa.h"
#include "isa/word.h"
#include "kdb/cmd.h"

#include <iostream>

static int cmd_info_gpr(const cmd::args_t &args);

static const cmd::cmd_map_t cmdMap = {
    {"gpr", cmd_info_gpr}
};

static int cmd_info_gpr(const cmd::args_t &args) {
    for (int i = 0; i < GPR_COUNT; i++) {
        word_t value = cmd::currentCore->get_gpr(i);
        std::cout << get_gpr_name(i) << " = " 
        << FMT_STREAM_WORD(value) 
        << "(" << value<< ")" << std::endl;
    }
    return cmd::Success;
}

int cmd::info(const args_t &args) {
    return cmd::find_and_run(args, cmdMap, 1);
}
