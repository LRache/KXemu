#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "memory/map.h"
#include <cstdint>
#include <string>
#include <vector>

class Memory {
public:
    uint64_t read(uint64_t addr, int size) const;
    bool write(uint64_t addr, uint64_t data, int size);

    bool add_memory_map(std::string name, uint64_t start, uint64_t length, MemoryMap *map);
    void free_all();

private:        
    struct MapBlock {
        std::string name;
        uint64_t start;
        uint64_t length;
        MemoryMap *map;
    };
    std::vector<MapBlock> memory_maps;
};

#endif
