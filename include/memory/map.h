#ifndef __MEMORY_MAP_H__
#define __MEMORY_MAP_H__

#include "isa/word.h"
#include <cstdint>
#include <string>

class MemoryMap {
public:
    virtual word_t read(word_t offset, int size) = 0;
    virtual bool write(word_t offset, word_t data, int size) = 0;
    virtual uint8_t *get_ptr(word_t offset) = 0;
    virtual std::string get_type_str() const = 0;
    virtual ~MemoryMap() {};
};


// For storage memory, like flash, sdram or sram
class StorageMemoryMap : public MemoryMap {
private:
    uint64_t length;
public:
    StorageMemoryMap(uint64_t length);
    ~StorageMemoryMap();
    
    word_t read(word_t addr, int size) override;
    bool write(word_t addr, word_t data, int size) override;
    uint8_t *get_ptr(word_t addr) override;
    std::string get_type_str() const override;

    uint8_t *data;
};

#endif
