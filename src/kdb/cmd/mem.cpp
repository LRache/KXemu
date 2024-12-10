#include "kdb/kdb.h"

#include <string>
#include <vector>
#include <iostream>

int cmd_mem_create(const std::vector<std::string> &);
int cmd_mem_img   (const std::vector<std::string> &);
int cmd_mem_elf   (const std::vector<std::string> &);

static const struct {
    std::string command;
    int (*fun)(const std::vector<std::string> &);
} commandMap[] = {
    {"create", cmd_mem_create},
    {"img"   , cmd_mem_img   },
    {"elf"   , cmd_mem_elf   }
};

static bool check_memory_initialized() {
    if (kdb::memory == nullptr) {
        std::cout << "Memory not initialized" << std::endl;
        return false;
    }
    return true;
}

int cmd_mem_create(const std::vector<std::string> &args) {
    if (!check_memory_initialized()) return kdb::MissingPrevOp;

    if (args.size() < 4) {
        std::cout << "Usage: mem create <name> <addr> <size> [type]" << std::endl;
        return kdb::InvalidArgs;
    }

    // TODO: parse args and create a memory map, call kdb::memory->create_map

    return 0;
}

int cmd_mem_img(const std::vector<std::string> &) {
    // TODO: parse args and load local file to memory by calling kdb::mem_load_img, see kdb docs
    return 0;
}

int cmd_mem_elf(const std::vector<std::string> &) {
    // TODO: parse args and load local file to memory by calling kdb::mem_load_elf, see kdb docs
    return 0;
}

int cmd_mem(const std::vector<std::string> &) {
    return 0;
}


