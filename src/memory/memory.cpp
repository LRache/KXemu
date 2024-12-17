#include "memory/memory.h"
#include "isa/word.h"
#include "log.h"
#include "memory/map.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ios>

#define BUFFER_SIZE 1024U

bool Memory::add_memory_map(const std::string &name, word_t start, word_t length, MemoryMap *map) {
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

void Memory::free_all() {
    for (auto &m : memoryMaps) {
        delete m->map;
        m->map = nullptr;
        delete m;
    }
    memoryMaps.clear();
}

word_t Memory::read(word_t addr, int size) const {
    auto map = match_map(addr);
    if (map != nullptr) {
        word_t data = map->map->read(addr - map->start, size);
        return data;
    }
    WARN("read addr: " FMT_WORD ", size: " FMT_VARU ", out of range", addr, size);
    return 0;
}

bool Memory::write(word_t addr, word_t data, int size) {
    auto map = match_map(addr);
    if (map != nullptr) {
        return map->map->write(addr - map->start, data, size);
    }
    return false;
}

uint8_t *Memory::get_ptr(word_t addr) const{
    auto map = match_map(addr);
    if (map == nullptr) {
        return nullptr;
    }
    return map->map->get_ptr(addr - map->start);
}

word_t Memory::get_ptr_length(word_t addr) const {
    auto map = match_map(addr);
    if (map == nullptr) {
        return 0;
    }
    word_t offset = addr - map->start;
    return map->length - offset;
}

bool Memory::load_from_stream(std::istream &stream, word_t addr) {
    auto map = match_map(addr);
    if (map == nullptr) {
        WARN("addr: " FMT_WORD "out of range", addr);
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

bool Memory::load_from_stream(std::istream &stream, word_t addr, word_t length) {
    auto map = match_map(addr);
    if (map == nullptr) {
        WARN("addr: " FMT_WORD "out of range", addr);
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
        WARN("load image file to memory size out of range, only write " FMT_VARU " bytes", leftLength);
    }

    uint8_t buffer[BUFFER_SIZE];
    word_t writen = 0;
    while (true) {
        word_t readLength = std::min(BUFFER_SIZE, length - writen);
        stream.read((char *)buffer, readLength);
        std::streamsize count = stream.gcount();
        if (count != readLength) {
            WARN("caught stream eof, only write " FMT_VARU " bytes", writen);
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

bool Memory::load_from_memory(const uint8_t *src, word_t addr, word_t length) {
    auto map = match_map(addr);
    if (map == nullptr) {
        WARN("addr=" FMT_WORD " out of range", addr);
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
        WARN("load image file to memory size out of range, only write " FMT_VARU " bytes", leftLength);
    }
    std::memcpy(dest, src, length);
    return true;
}

bool Memory::memset(word_t addr, word_t length, uint8_t byte) {
    auto map = match_map(addr);
    if (map == nullptr || addr + length > map->start + map->length) {
        WARN("memset addr=" FMT_WORD " length=" FMT_VARU " out of range", addr, length);
        return false;
    }

    word_t offset = addr - map->start;
    void *dest = map->map->get_ptr(offset);
    std::memset(dest, byte, length);

    return true;
}

bool Memory::dump(std::ostream &stream, word_t addr, word_t length) const {
    auto map = match_map(addr);
    if (map == nullptr || addr + length > map->start + map->length) {
        WARN("dump addr=" FMT_WORD " length=" FMT_VARU " out of range", addr, length);
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
        WARN("dump memory size out of range, only dump " FMT_VARU " bytes", leftLength);
    }

    stream.write((const char *)src, length);
    return true;
}

Memory::MapBlock *Memory::match_map(word_t addr) const {
    for (size_t i = 0; i < memoryMaps.size(); i++) {
        if (memoryMaps[i]->start <= addr && addr < memoryMaps[i]->start + memoryMaps[i]->length) {
            return memoryMaps[i];
        }
    }
    return nullptr;
}

Memory::~Memory() {
    for (auto &m : memoryMaps) {
        delete m;
    }
    memoryMaps.clear();
}
