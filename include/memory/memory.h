#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "memory/map.h"
#include "common.h"
#include <cstdint>
#include <string>
#include <vector>

class Memory {
public:
    word_t read(word_t addr, int size) const;
    bool write(word_t addr, word_t data, int size);
    uint8_t *get_ptr(word_t addr);

    bool add_memory_map(std::string name, word_t start, word_t length, MemoryMap *map);
    void free_all();

private:        
    struct MapBlock {
        std::string name;
        word_t start;
        word_t length;
        MemoryMap *map;
    };
    std::vector<MapBlock *> memory_maps;
    MapBlock *match_map(word_t addr) const;
};

#endif
