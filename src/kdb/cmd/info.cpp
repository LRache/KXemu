#include "isa/isa.h"
#include "kdb/cmd.h"

#include <iostream>

static int cmd_info_gpr(const cmd::args_t &args) {
    for (int i = 0; i < GPR_COUNT; i++) {
        std::cout << get_gpr_name(i) << " = " << cmd::currentCore->get_gpr(i) << std::endl;
    }
    return cmd::Success;
}

int cmd::info(const args_t &args) {
    return cmd::Success;
}
