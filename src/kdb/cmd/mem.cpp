#include "kdb/kdb.h"

#include <vector>
#include <iostream>

int cmd_mem_create(std::vector<std::string> &args) {
    if (kdb::memory == nullptr) {
        std::cout << "Memory not initialized" << std::endl;
        return 1;
    }

    if (args.size() < 4) {
        std::cout << "Usage: mem create <name> <addr> <size> [type]" << std::endl;
        return 1;
    }

    return 0;
}

int cmd_mem(std::vector<std::string> &args) {
    return 0;
}


