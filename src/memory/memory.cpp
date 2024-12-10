#include "memory/memory.h"
#include "common.h"
#include "log.h"
#include "memory/map.h"
#include <cstddef>
#include <cstdint>

bool Memory::add_memory_map(std::string name, word_t start, word_t length, MemoryMap *map) {
    // check if overlap
    for (auto &m : memory_maps) {
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
    memory_maps.push_back(m);
    return true;
}

void Memory::free_all() {
    for (auto &m : memory_maps) {
        delete m->map;
        m->map = nullptr;
        delete m;
    }
    memory_maps.clear();
}

word_t Memory::read(word_t addr, int size) const {
    auto map = match_map(addr);
    if (map != nullptr) {
        return map->map->read(addr - map->start, size);
    }
    WARN("read addr: " FMT_WORD ", size: %d, out of range", addr, size);
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

Memory::MapBlock *Memory::match_map(word_t addr) const {
    for (size_t i = 0; i < memory_maps.size(); i++) {
        if (memory_maps[i]->start <= addr && addr < memory_maps[i]->start + memory_maps[i]->length) {
            return memory_maps[i];
        }
    }
    return nullptr;
}
