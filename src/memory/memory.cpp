#include "memory/memory.h"
#include "common.h"
#include "log.h"
#include "memory/map.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>

bool Memory::add_memory_map(std::string name, word_t start, word_t length, MemoryMap *map) {
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
        return map->map->read(addr - map->start, size);
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

uint8_t *Memory::get_ptr(word_t addr) {
    auto map = match_map(addr);
    if (map != nullptr) {
        return map->map->get_ptr(addr - map->start);
    }
    return nullptr;
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
    
    uint8_t *end  = map->map->get_ptr(0) + map->length;
    uint8_t byte;
    while (true) {
        stream.read(reinterpret_cast<char *>(&byte), 1);
        if (stream.eof()) {
            break;
        } else {
            *dest = byte;
            dest ++;
            if (dest == end) {
                WARN("load image file to memory size out of range, only write %ld bytes", dest - map->map->get_ptr(offset));
                break;
            }
        }
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
    if (length < leftLength) {
        WARN("load image file to memory size out of range, only write " FMT_VARU " bytes", leftLength);
    }
    word_t writen = 0;
    uint8_t byte;
    while (true) {
        stream.read(reinterpret_cast<char *>(&byte), 1);
        if (writen == length) {
            break;
        }
        if (stream.eof()) {
            WARN("caught stream eof, only write " FMT_VARU " bytes", writen);
            break;
        }
        writen ++;
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
    if (length < leftLength) {
        WARN("load image file to memory size out of range, only write " FMT_VARU " bytes", leftLength);
    }
    std::memcpy(dest, src, length);
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
