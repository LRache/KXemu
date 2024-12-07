#ifndef __MEMORY_MAP_H__
#define __MEMORY_MAP_H__

#include <cstdint>

class MemoryMap {
public:
    virtual uint64_t read(uint64_t offset, int size) const = 0;
    virtual bool write(uint64_t offset, uint64_t data, int size) = 0;
    virtual ~MemoryMap() {};
};


// for storage memory, like flash, dram or sram
class StorageMemoryMap : public MemoryMap {
private:
    uint64_t length;
public:
    StorageMemoryMap(uint64_t length);
    ~StorageMemoryMap();
    
    uint64_t read(uint64_t addr, int size) const;
    bool write(uint64_t addr, uint64_t data, int size);

    uint8_t *data;
};

#endif
