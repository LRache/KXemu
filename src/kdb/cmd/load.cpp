#include "common.h"
#include "kdb/kdb.h"
#include "kdb/cmd.h"

#include <iostream>

std::string cmd::elfFileName;

static int cmd_load_elf(const cmd::args_t &);

static const cmd::cmd_map_t cmdMap = {
    {"elf", cmd_load_elf},
};

static int cmd_load_elf(const cmd::args_t &args) {
    if (args.size() > 2) {
        std::cout << "WARN: load elf command only get source from kdb exec arguments." << std::endl;
    }
    if (cmd::elfFileName.empty()) {
        std::cout << "Failed to load ELF file, because file name is empty." << std::endl;
        return cmd::MissingPrevOp;
    }
    
    word_t entry = kdb::load_elf(cmd::elfFileName);
    if (entry == 0) {
        std::cout << "Failed to load ELF file " << cmd::elfFileName << std::endl;
        return cmd::CmdError;
    }
    std::cout << "Load ELF file success." << std::endl;
    std::cout << "Switch entry to " << FMT_STREAM_WORD(entry) << "." << std::endl;
    kdb::programEntry = entry;
    return cmd::Success;
}

int cmd::load(const cmd::args_t & args) {
    int r = cmd::find_and_run(args, cmdMap, 1);
    if (r == cmd::EmptyArgs) {
        std::cout << "Usage: load elf" << std::endl;
    }
    return r;
}
