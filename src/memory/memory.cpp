#include "memory/memory.h"
#include "log.h"
#include <cstdint>

bool Memory::add_memory_map(std::string name, uint64_t start, uint64_t length, MemoryMap *map) {
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

uint64_t Memory::read(uint64_t addr, int size) const {
    DEBUG("read addr: 0x%lx, size: %d", addr, size);
    for (auto &m : memory_maps) {
        if (m.start <= addr && addr < m.start + m.length) {
            return m.map->read(addr - m.start, size);
        }
    }
    WARN("read addr: 0x%lx, size: %d, out of range", addr, size);
    return 0;
}

bool Memory::write(uint64_t addr, uint64_t data, int size) {
    for (auto &m : memory_maps) {
        if (m.start <= addr && addr < m.start + m.length) {
            return m.map->write(addr - m.start, data, size);
        }
    }
    return false;
}
