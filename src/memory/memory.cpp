#include "memory/memory.h"
#include "common.h"
#include "log.h"

bool Memory::add_memory_map(std::string name, word_t start, word_t length, MemoryMap *map) {
    // check if overlap
    for (auto &m : memory_maps) {
        if (m.start <= start && start < m.start + m.length) {
            return false;
        }
        if (start <= m.start && m.start < start + length) {
            return false;
        }
    }
    MapBlock m = {name, start, length, map};
    memory_maps.push_back(m);
    return true;
}

void Memory::free_all() {
    for (auto &m : memory_maps) {
        delete m.map;
        m.map = nullptr;
    }
    memory_maps.clear();
}

word_t Memory::read(word_t addr, int size) const {
    // DEBUG("read addr: " FMT_WORD ", size: %d", addr, size);
    for (auto &m : memory_maps) {
        if (m.start <= addr && addr < m.start + m.length) {
            return m.map->read(addr - m.start, size);
        }
    }
    WARN("read addr: " FMT_WORD ", size: %d, out of range", addr, size);
    return 0;
}

bool Memory::write(word_t addr, word_t data, int size) {
    for (auto &m : memory_maps) {
        if (m.start <= addr && addr < m.start + m.length) {
            return m.map->write(addr - m.start, data, size);
        }
    }
    return false;
}
