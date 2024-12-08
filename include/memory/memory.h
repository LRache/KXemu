#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "memory/map.h"
#include "common.h"
#include <string>
#include <vector>

class Memory {
public:
    word_t read(word_t addr, int size) const;
    bool write(word_t addr, word_t data, int size);

    bool add_memory_map(std::string name, word_t start, word_t length, MemoryMap *map);
    void free_all();

private:        
    struct MapBlock {
        std::string name;
        word_t start;
        word_t length;
        MemoryMap *map;
    };
    std::vector<MapBlock> memory_maps;
};

#endif
