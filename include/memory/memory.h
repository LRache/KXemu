#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "memory/map.h"
#include "common.h"
#include <cstdint>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <vector>

class Memory {
public:
    word_t read(word_t addr, int size) const;
    bool write(word_t addr, word_t data, int size);
    uint8_t *get_ptr(word_t addr);

    bool load_from_stream(std::istream &stream, word_t addr);
    bool load_from_stream(std::istream &stream, word_t addr, word_t length);
    bool load_from_memory(const uint8_t *src, word_t addr, word_t length);
    bool memset(word_t addr, word_t length, uint8_t byte);

    bool add_memory_map(std::string name, word_t start, word_t length, MemoryMap *map);
    void free_all();
       
    struct MapBlock {
        std::string name;
        word_t start;
        word_t length;
        MemoryMap *map;
    };
    std::vector<MapBlock *> memoryMaps;
    MapBlock *match_map(word_t addr) const;
};

#endif
