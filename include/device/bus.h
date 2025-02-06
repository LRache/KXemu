#ifndef __KXEMU_DEVICE_BUS_H__
#define __KXEMU_DEVICE_BUS_H__

#include <vector>
#include <string>
#include <cstdint>

namespace kxemu::device {

    using word_t = uint64_t;

enum AMO {
    AMO_SWAP,
    AMO_ADD,
    AMO_AND,
    AMO_OR,
    AMO_XOR,
    AMO_MIN,
    AMO_MAX,
    AMO_MINU,
    AMO_MAXU
};

class MemoryMap {
public:
    virtual ~MemoryMap() {};

    virtual word_t read(word_t offset, int size) = 0;
    virtual bool write(word_t offset, word_t data, int size) = 0;
    
    virtual word_t do_atomic(word_t addr, word_t data, int size, AMO amo, bool &valid) {
        valid = false;
        return 0;
    }
   
    virtual uint8_t *get_ptr(word_t offset) = 0;
    
    virtual const char *get_type_name() const = 0;
};

class Bus {
public:
    ~Bus();

    struct MapBlock {
        std::string name;
        word_t start;
        word_t length;
        MemoryMap *map;
    };
    std::vector<MapBlock *> memoryMaps;
    MapBlock *match_map(word_t addr, word_t size = 0) const;
    bool add_memory_map(const std::string &name, word_t start, word_t length, MemoryMap *map);
    void free_all();

    word_t read(word_t addr, unsigned int size, bool &valid) const;
    bool write(word_t addr, word_t data, int size);

    word_t do_atomic(word_t addr, word_t data, int size, AMO amo, bool &valid);

    bool load_from_stream(std::istream &stream, word_t addr);
    bool load_from_stream(std::istream &stream, word_t addr, word_t length);
    bool load_from_memory(const uint8_t *src, word_t addr, word_t length);
    
    bool dump(std::ostream &stream, word_t addr, word_t length) const;
    
    bool memset(word_t addr, word_t length, uint8_t byte);
    uint8_t *get_ptr(word_t addr) const;
    word_t   get_ptr_length(word_t addr) const;
};

} // namespace kxemu::device

#endif
