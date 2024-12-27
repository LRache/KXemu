#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "isa/word.h"

#include <vector>

class MemoryMap {
public:
    virtual word_t read(word_t offset, int size) = 0;
    virtual bool write(word_t offset, word_t data, int size) = 0;
    virtual uint8_t *get_ptr(word_t offset) = 0;
    virtual std::string get_type_str() const = 0;
    virtual ~MemoryMap() {};
};

class Bus {
public:
    word_t read(word_t addr, int size, bool &valid) const;
    bool write(word_t addr, word_t data, int size);
    uint8_t *get_ptr(word_t addr) const;
    word_t   get_ptr_length(word_t addr) const;

    bool load_from_stream(std::istream &stream, word_t addr);
    bool load_from_stream(std::istream &stream, word_t addr, word_t length);
    bool load_from_memory(const uint8_t *src, word_t addr, word_t length);
    
    bool dump(std::ostream &stream, word_t addr, word_t length) const;
    bool memset(word_t addr, word_t length, uint8_t byte);

    bool add_memory_map(const std::string &name, word_t start, word_t length, MemoryMap *map);
    void free_all();
       
    struct MapBlock {
        std::string name;
        word_t start;
        word_t length;
        MemoryMap *map;
    };
    std::vector<MapBlock *> memoryMaps;
    MapBlock *match_map(word_t addr, word_t size = 0) const;

    ~Bus();
};

#endif
