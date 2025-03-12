#ifndef __KXEMU_DEVICE_BUS_H__
#define __KXEMU_DEVICE_BUS_H__

#include "device/mmio.h"
#include "device/def.h"
#include "utils/spinlock.h"

#include <vector>
#include <iostream>
#include <cstdint>

namespace kxemu::device {

class Bus {
private:
    utils::SpinLock updateLock;

public:
    ~Bus();

    struct MemoryBlock {
        word_t start;
        word_t end;
        uint8_t *data;
    };
    std::vector<MemoryBlock *> memoryMaps;

    struct MMIOMapBlock {
        word_t start;
        word_t size;
        unsigned int id = 0;
        MMIODev *dev;
    };
    std::vector<MMIOMapBlock *> mmioMaps;

    MMIOMapBlock *match_mmio  (word_t addr, word_t length = 0) const;
    MemoryBlock  *match_memory(word_t addr, word_t length = 0) const;
    bool add_mmio_map  (unsigned int id, word_t start, word_t length, MMIODev *map);
    bool add_mmio_map  (word_t start, word_t size, MMIODev *map);
    bool add_memory_map(word_t start, word_t size);

    word_t read (word_t addr, word_t length, bool &valid) const;
    bool   write(word_t addr, word_t data, word_t length);
    void update();

    word_t do_atomic(word_t addr, word_t data, word_t length, AMO amo, bool &valid);

    bool load_from_stream(std::istream &stream, word_t addr);
    bool load_from_stream(std::istream &stream, word_t addr, word_t length);
    bool load_from_memory(void *src, word_t addr, word_t length);
    
    bool dump(std::ostream &stream, word_t addr, word_t length) const;
    
    bool memset(word_t addr, word_t length, uint8_t byte);
    bool memcpy(word_t addr, word_t length, void *dest);
    void *get_ptr(word_t addr) const;
    void *get_ptr(word_t addr, word_t length) const;
    word_t get_ptr_length(word_t addr) const;
};

} // namespace kxemu::device

#endif
