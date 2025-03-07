#include "device/virtio/block.h"
#include "kdb/cmd.h"
#include "kdb/kdb.h"
#include "utils/utils.h"
#include "word.h"

#include <iostream>
#include <iomanip>

using namespace kxemu;
using namespace kxemu::kdb;

static int device_ls (const cmd::args_t &args);
static int device_add(const cmd::args_t &args);

static const cmd::cmd_map_t cmdMap = {
    {"ls",  device_ls },
    {"add", device_add}
};

static int device_ls(const cmd::args_t &args) {
    std::cout << std::setfill(' ')
    << std::setw(4) << "id"   << " | "
    << std::setw(8) << "type" << " | "
    << std::setw(WORD_WIDTH + 2) << "start" << " | "
    << std::setw(WORD_WIDTH + 2) << "end"   << " | "
    << std::setw(WORD_WIDTH + 2) << "size"  << " | "
    << std::endl;
    
    for (const auto &memoryDevice : kdb::bus->memoryMaps) {
        std::cout << std::setfill(' ')
        << std::setw(4) << "0" << " | "
        << std::setw(8) << "Memory" << " | "
        << FMT_STREAM_WORD_SPACE(memoryDevice->start)  << " | "
        << FMT_STREAM_WORD_SPACE(memoryDevice->end)    << " | "
        << FMT_STREAM_WORD_SPACE(memoryDevice->end - memoryDevice->start) << " | "
        << std::endl;
    }

    for (const auto &ioDevice : kdb::bus->mmioMaps) {
        std::cout << std::setfill(' ')
        << std::setw(4) << ioDevice->id << " | "
        << std::setw(8) << ioDevice->map->get_type_name() << " | "
        << FMT_STREAM_WORD_SPACE(ioDevice->start)  << " | "
        << FMT_STREAM_WORD_SPACE(ioDevice->start + ioDevice->size) << " | "
        << FMT_STREAM_WORD_SPACE(ioDevice->size) << " | "
        << std::endl;
    }

    return cmd::Success;
}

static int device_add_uart(unsigned int id, const cmd::args_t &args) {
    if (args.size() < 5) {
        std::cout << "Usage: device add [id] uart <base> <ip> <port>" << std::endl;
        return cmd::EmptyArgs;
    }

    auto base = utils::string_to_unsigned(args[4]);
    if (!base.has_value()) {
        std::cout << "Invalid args: " << args[4] << std::endl;
        return cmd::InvalidArgs;
    }

    bool success;
    if (args.size() == 5) {
        // [0]device [1]add [2]uart [3]id [4]base
        success = kdb::uart::add(id, base.value(), std::cout);
    } else if (args.size() == 6) {
        // [0]device [1]add [2]uart [3]id [4]base [5]port
        int port = std::stoi(args[5]);
        std::cout << "uart.ip=" << "127.0.0.1" << ", uart.port=" << port << std::endl;
        success =  kdb::uart::add(id, base.value(), "127.0.0.1", port);
    } else {
        // [0]device [1]add [2]uart [3]id [4]base [5]ip [6]port
        success = kdb::uart::add(id, base.value(), args[4], std::stoi(args[5]));
    }

    if (!success) {
        std::cout << "Failed to add uart device" << std::endl;
        return cmd::CmdError;
    } 

    std::cout << "Add uart device: id=" << id << ", base=" << FMT_STREAM_WORD(base.value()) << std::endl;
    return cmd::Success;
}

static int device_add_virtio_block(unsigned int id, const cmd::args_t &args) {
    // [0]device [1]add [2]id [3]virtio-blk [4]base [5]img-type [6]img-filepath
    if (args.size() < 7) {
        return cmd::InvalidArgs;
    }

    auto base = utils::string_to_unsigned(args[4]);
    if (!base.has_value()) {
        std::cerr << "Invalid base: " << args[4] << std::endl;
        return cmd::InvalidArgs;
    }
    const std::string &imgType = args[5];
    const std::string &filepath = args[6];

    device::VirtIOBlock *dev = new device::VirtIOBlock;
    if (imgType == "raw") {
        if (!dev->open_raw_img(filepath)) {
            std::cerr << "Failed to open image file: " << imgType << std::endl;
            delete dev;
            return cmd::CmdError;
        }
    } else {
        std::cerr << "Unknown image type: " << imgType << std::endl;
        delete dev;
        return cmd::InvalidArgs;
    }
    
    if (!bus->add_mmio_map(id, base.value(), 0x200, dev)) {
        std::cerr << "Failed to add device."  << std::endl;
        return cmd::CmdError;
    }

    std::cout << "Add virtio-block device: id=" << id << ", base=" << FMT_STREAM_WORD(base.value()) << std::endl;

    return cmd::Success;
}

static int device_add(const cmd::args_t &args) {
    if (args.size() < 4) {
        std::cout << "Usage: device add <type> <start> <size>" << std::endl;
        return cmd::EmptyArgs;
    }

    if (args[2] == "memory") {
        if (args.size() < 6) {
            std::cout << "Usage: device add memory <start> <size>" << std::endl;
            return cmd::EmptyArgs;
        }
        
        auto start = utils::string_to_unsigned(args[3]);
        auto size = utils::string_to_unsigned(args[4]);
        if (!start.has_value() || !size.has_value()) {
            std::cout << "Invalid args" << std::endl;
            return cmd::InvalidArgs;
        }
        
        kdb::bus->add_memory_map(start.value(), size.value());
    } else {
        auto id = utils::string_to_unsigned(args[2]);
        if (!id.has_value()) {
            std::cout << "Invalid args: " << args[2] << std::endl;
            return cmd::InvalidArgs;
        }
        
        const std::string &type = args[3];
        if (type == "uart") {
            device_add_uart(id.value(), args);
        } else if (type == "virtio-blk") {
            device_add_virtio_block(id.value(), args);
        } else {
            std::cerr << "Unknown device type: " << type << std::endl;
            return cmd::InvalidArgs;
        }
    }

    return cmd::Success;
}

int cmd::device(const args_t &args) {
    return find_and_run(args, cmdMap, 1);
}
