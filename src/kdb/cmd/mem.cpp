#include "common.h"
#include "kdb/kdb.h"
#include "kdb/cmd.h"
#include "memory/map.h"
#include "utils/utils.h"

#include <algorithm>
#include <exception>
#include <iomanip>
#include <ostream>
#include <string>
#include <vector>
#include <iostream>

static int cmd_mem_create(const cmd::args_t &);
static int cmd_mem_img   (const cmd::args_t &);
static int cmd_mem_elf   (const cmd::args_t &);
static int cmd_mem_map   (const cmd::args_t &);

static const cmd::cmd_map_t cmdMap = {
    {"create"   , cmd_mem_create},
    {"img"      , cmd_mem_img   },
    {"elf"      , cmd_mem_elf   },
    {"map"      , cmd_mem_map   }
};

static bool check_memory_initialized() {
    if (kdb::memory == nullptr) {
        std::cout << "Memory not initialized" << std::endl;
        return false;
    }
    return true;
}

static int cmd_mem_create(const std::vector<std::string> &args) {
    if (!check_memory_initialized()) return cmd::MissingPrevOp;

    if (args.size() < 5) {
        std::cout << "Usage: mem create <name> <addr> <size> [type]" << std::endl;
        return cmd::InvalidArgs;
    }

    // TODO: implement memory map type argument
    std::string name;
    word_t start, size;
    name = args[2];
    try {
        start = utils::string_to_word(args[3]);
        size = utils::string_to_word(args[4]);
    } catch (std::exception &) {
        std::cout << "Usage: mem create <name> <addr> <size> [type]" << std::endl;
        return cmd::InvalidArgs;
    }
    
    bool s = kdb::memory->add_memory_map(name, start, size, new StorageMemoryMap(size));
    if (s) {
        std::cout << "Create new memory map " << name << " at " << FMT_STREAM_WORD(start) << " with size=" << size << std::endl;
        return cmd::Success;
    } else {
        return cmd::CmdError;
    }
}

static int cmd_mem_img(const cmd::args_t &) {
    // TODO: parse args and load local file to memory by calling kdb::mem_load_img, see kdb docs
    return 0;
}

// load elf to memory and switch the program entry
static int cmd_mem_elf(const cmd::args_t &args) {
    if (args.size() < 3) {
        std::cout << "Missing elf file path" << std::endl;
        return cmd::InvalidArgs;
    }
    std::string filename = args[2];
    word_t entry = kdb::load_elf(filename);
    if (entry == 0) {
        std::cout << "Error when load elf files" << std::endl;
        return cmd::CmdError;
    }
    std::cout << "Load ELF file success." << std::endl;
    std::cout << "Switch entry to " << FMT_STREAM_WORD(entry) << "." << std::endl;
    kdb::programEntry = entry;
    return cmd::Success;
}

static int cmd_mem_map(const cmd::args_t &) {
    if (kdb::memory->memoryMaps.empty()) {
        std::cout << "There is no any memory map." << std::endl;
        return cmd::Success;
    }

    std::cout << "Memory mapping info" << std::endl;
    std::cout << std::setfill(' ')
    << std::setw(10)  << "name" << " | "
    << std::setw(WORD_WIDTH + 2) << "start" << " | "
    << std::setw(WORD_WIDTH + 2) << "size" << " | "
    << "type"
    << std::setw(8) << std::endl;

    for (auto &m : kdb::memory->memoryMaps) {
        std::cout << std::setfill(' ')
        << std::setw(10) << m->name << " | "
        << FMT_STREAM_WORD(m->start)  << " | "
        << FMT_STREAM_WORD(m->length) << " | "
        << std::setw(6) << m->map->get_type_str() << std::endl;
    }
    return cmd::Success;
}

int cmd::mem(const cmd::args_t &args) {
    int r = cmd::find_and_run(args, cmdMap, 1);
    if (r == EmptyArgs) {
        std::cout << "Usage: mem <create|img|elf>" << std::endl;
        return Success;
    } else if (r == CmdError) {
        std::cout << "Error running command " << args[1] << std::endl;
    } else if (r == InvalidArgs) {
        std::cout << "Invalid arguments for command " << args[1] << std::endl;
    }
    return r;
}


