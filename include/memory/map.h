#ifndef __MEMORY_MAP_H__
#define __MEMORY_MAP_H__

#include "common.h"

class MemoryMap {
public:
    virtual word_t read(word_t offset, int size) const = 0;
    virtual bool write(word_t offset, word_t data, int size) = 0;
    virtual ~MemoryMap() {};
};


// for storage memory, like flash, dram or sram
class StorageMemoryMap : public MemoryMap {
private:
    uint64_t length;
public:
    StorageMemoryMap(uint64_t length);
    ~StorageMemoryMap();
    
    word_t read(word_t addr, int size) const;
    bool write(word_t addr, word_t data, int size);

    uint8_t *data;
};

#endif
