#include "kdb/kdb.h"
#include "kdb/cmd.h"

#include <iostream>

using namespace kxemu;

int kdb::cmd::gdb(const args_t &args) {
    if (args.size() != 2) {
        std::cout << "Usage: gdb <addr>" << std::endl;
        return InvalidArgs;
    }

    if (!kdb::run_gdb(args[1])) {
        std::cout << "Error when run gdb rsp." << std::endl;
        return CmdError;
    }

    return Success;
}
