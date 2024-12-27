#ifndef __DEVICE_MEMORY_H__
#define __DEVICE_MEMORY_H__

#include "device/bus.h"

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
