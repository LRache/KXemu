#include "log.h"
#include "device/bus.h"

#include <algorithm>
#include <istream>
#include <cstdint>
#include <cstring>

#define BUFFER_SIZE ((word_t)1024)

using namespace kxemu::device;

Bus::MapBlock *Bus::match_map(word_t addr, word_t size) const {
    for (auto &m : memoryMaps) {
        if (m->start <= addr && addr + size <= m->start + m->length) {
            return m;
        }
    }
    return nullptr;
}

bool Bus::add_memory_map(const std::string &name, word_t start, word_t length, MemoryMap *map) {
    // check if overlap
    for (auto &m : memoryMaps) {
        if (m->start <= start && start < m->start + m->length) {
            return false;
        }
        if (start <= m->start && m->start < start + length) {
            return false;
        }
    }
    MapBlock *m = new MapBlock();
    m->name = name;
    m->start = start;
    m->length = length;
    m->map = map;
    memoryMaps.push_back(m);
    return true;
}

void Bus::free_all() {
    for (auto &m : memoryMaps) {
        delete m->map;
        m->map = nullptr;
        delete m;
    }
    memoryMaps.clear();
}

word_t Bus::read(word_t addr, unsigned int size, bool &valid) const {
    auto map = match_map(addr, size);
    if (map != nullptr) {
        valid = true;
        word_t data = map->map->read(addr - map->start, size);
        return data;
    }
    valid = false;
    PANIC("read addr: 0x%" PRIx64 ", size: %" PRIu32 ", out of range", addr, size);
    return 0;
}

bool Bus::write(word_t addr, word_t data, int size) {
    auto map = match_map(addr, size);
    if (map != nullptr) {
        return map->map->write(addr - map->start, data, size);
    }
    return false;
}

word_t Bus::do_atomic(word_t addr, word_t data, int size, AMO amo, bool &valid) {
    auto map = match_map(addr, size);
    if (map == nullptr) {
        valid = false;
        return 0;
    }
    return map->map->do_atomic(addr - map->start, data, size, amo, valid);
}

bool Bus::load_from_stream(std::istream &stream, word_t addr) {
    auto map = match_map(addr);
    if (map == nullptr) {
        WARN("addr: 0x%" PRIx64 "out of range", addr);
        return false;
    }
    word_t offset = addr - map->start;
    uint8_t *dest = map->map->get_ptr(offset);
    if (dest == nullptr) {
        WARN("Unable to write to destination %s", map->name.c_str());
        return false;
    }

    uint8_t buffer[BUFFER_SIZE];
    word_t writen = 0;
    word_t maxLength = map->length - offset; 
    std::streamsize count = BUFFER_SIZE;
    while (count == BUFFER_SIZE) {
        stream.read((char *)buffer, BUFFER_SIZE);
        count = stream.gcount();
        if (writen + count >= maxLength) {
            auto left = maxLength - writen;
            std::memcpy(dest, buffer, left);
            WARN("load image file to memory size out of range, only write %ld bytes", dest - map->map->get_ptr(offset));
            break;
        }
        std::memcpy(dest, buffer, count);
        dest += count;
        writen += count;
    }
    return true;
}

bool Bus::load_from_stream(std::istream &stream, word_t addr, word_t length) {
    auto map = match_map(addr);
    if (map == nullptr) {
        WARN("addr: " FMT_HEX64 "out of range", addr);
        return false;
    }
    word_t offset = addr - map->start;
    uint8_t *dest = map->map->get_ptr(offset);
    if (dest == nullptr) {
        WARN("Unable to write to destination %s", map->name.c_str());
        return false;
    }
    
    word_t leftLength = map->length - offset;
    if (length > leftLength) {
        WARN("load image file to memory size out of range, only write " FMT_VARU64 " bytes", leftLength);
    }

    uint8_t buffer[BUFFER_SIZE];
    word_t writen = 0;
    while (true) {
        std::streamsize readLength = std::min(BUFFER_SIZE, length - writen);
        stream.read((char *)buffer, readLength);
        std::streamsize count = stream.gcount();
        if (count != readLength) {
            WARN("caught stream eof, only write " FMT_VARU64 " bytes", writen);
            break;
        }
        std::memcpy(dest, buffer, count);
        dest += count;
        writen += count;
        if (writen == length) {
            break;
        }
    }
    return true;
}

bool Bus::load_from_memory(const uint8_t *src, word_t addr, word_t length) {
    auto map = match_map(addr);
    if (map == nullptr) {
        WARN("addr=" FMT_HEX64 " out of range", addr);
        return false;
    }
    word_t offset = addr - map->start;
    uint8_t *dest = map->map->get_ptr(offset);
    if (dest == nullptr) {
        WARN("Unable to write to destination %s", map->name.c_str());
        return false;
    }

    word_t leftLength = map->length - offset;
    if (length > leftLength) {
        WARN("load image file to memory size out of range, only write " FMT_VARU64 " bytes", leftLength);
    }
    std::memcpy(dest, src, length);
    return true;
}

bool Bus::dump(std::ostream &stream, word_t addr, word_t length) const {
    auto map = match_map(addr);
    if (map == nullptr || addr + length > map->start + map->length) {
        WARN("dump addr=" FMT_HEX64 " length=" FMT_VARU64 " out of range", addr, length);
        return false;
    }

    word_t offset = addr - map->start;
    const uint8_t *src = map->map->get_ptr(offset);
    if (src == nullptr) {
        WARN("Unable to read from source %s", map->name.c_str());
        return false;
    }

    word_t leftLength = map->length - offset;
    if (length > leftLength) {
        WARN("dump memory size out of range, only dump " FMT_VARU64 " bytes", leftLength);
    }

    stream.write((const char *)src, length);
    return true;
}

bool Bus::memset(word_t addr, word_t length, uint8_t byte) {
    auto map = match_map(addr);
    if (map == nullptr || addr + length > map->start + map->length) {
        WARN("memset addr=" FMT_HEX64 " length=" FMT_VARU64 " out of range", addr, length);
        return false;
    }

    word_t offset = addr - map->start;
    void *dest = map->map->get_ptr(offset);
    std::memset(dest, byte, length);

    return true;
}

uint8_t *Bus::get_ptr(word_t addr) const{
    auto map = match_map(addr);
    if (map == nullptr) {
        return nullptr;
    }
    return map->map->get_ptr(addr - map->start);
}

word_t Bus::get_ptr_length(word_t addr) const {
    auto map = match_map(addr);
    if (map == nullptr) {
        return 0;
    }
    word_t offset = addr - map->start;
    return map->length - offset;
}

Bus::~Bus() {
    for (auto &m : memoryMaps) {
        delete m;
    }
    memoryMaps.clear();
}
