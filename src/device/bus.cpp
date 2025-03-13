#include "device/def.h"
#include "log.h"
#include "macro.h"
#include "word.h"
#include "device/bus.h"

#include <istream>
#include <new>
#include <cstdint>
#include <cstring>

#define BUFFER_SIZE ((word_t)1024)

using namespace kxemu::device;

Bus::MemoryBlock *Bus::match_memory(word_t addr, word_t length) const {
    for (auto &m : memoryMaps) {
        if (m->start <= addr && addr + length <= m->end) {
            return m;
        }
    }
    return nullptr;
}

Bus::MMIOMapBlock *Bus::match_mmio(word_t addr, word_t length) const {
    for (auto &m : mmioMaps) {
        if (m->start <= addr && addr + length <= m->start + m->size) {
            return m;
        }
    }
    return nullptr;
}

bool Bus::add_memory_map(word_t start, word_t size) {
    // check if overlap
    for (auto &m : memoryMaps) {
        if (m->start <= start && start < m->end) {
            return false;
        }
        if (start <= m->start && m->start < start + size) {
            return false;
        }
    }
    
    auto m = new MemoryBlock;
    m->start = start;
    m->end = start + size;
    m->data = new((std::align_val_t)8) uint8_t[size];
    memoryMaps.push_back(m);
    
    return true;
}

bool Bus::add_mmio_map(unsigned int id, word_t start, word_t length, MMIODev *dev) {
    // check if overlap
    for (auto &m : mmioMaps) {
        if (m->start <= start && start < m->start + m->size) {
            return false;
        }
        if (start <= m->start && m->start < start + length) {
            return false;
        }
    }

    auto m = new MMIOMapBlock;
    m->start = start;
    m->size = length;
    m->dev = dev;
    m->id = id;
    mmioMaps.push_back(m);
    dev->connect_to_bus(this);
    return true;
}

bool Bus::add_mmio_map(word_t start, word_t length, MMIODev *map) {
    return this->add_mmio_map(0, start, length, map);
}

word_t Bus::read(word_t addr, word_t length, bool &valid) const {
    valid = true;
    
    auto mem = this->match_memory(addr, length);
    if (likely(mem != nullptr)) {
        word_t offset = addr - mem->start;
        word_t data;
        switch (length) {
            case 1: data = *(uint8_t  *)(mem->data + offset); break;
            case 2: data = *(uint16_t *)(mem->data + offset); break;
            case 4: data = *(uint32_t *)(mem->data + offset); break;
            case 8: data = *(uint64_t *)(mem->data + offset); break;
            default: PANIC("Invalid length=" FMT_VARU64, length);
        }
        return data;
    }

    auto map = this->match_mmio(addr, length);
    if (likely(map != nullptr)) {
        return map->dev->read(addr - map->start, length, valid);
    }

    valid = false;
    return 0;
}

bool Bus::write(word_t addr, word_t data, word_t length) {
    auto mem = this->match_memory(addr, length);
    if (likely(mem != nullptr)) {
        word_t offset = addr - mem->start;
        switch (length) {
            case 1: *(uint8_t  *)(mem->data + offset) = data; break;
            case 2: *(uint16_t *)(mem->data + offset) = data; break;
            case 4: *(uint32_t *)(mem->data + offset) = data; break;
            case 8: *(uint64_t *)(mem->data + offset) = data; break;
            default: PANIC("Invalid length=" FMT_VARU64, length);
        }
        return true;
    }

    auto map = this->match_mmio(addr, length);
    if (map != nullptr) {
        return map->dev->write(addr - map->start, data, length);
    }
    return false;
}

void Bus::update() {
    for (auto &m : mmioMaps) {
        m->dev->update();
    }
}

#define AMO_FUNC(name, op) \
    switch (length) { \
        case 1: return op(( uint8_t *)p, data, __ATOMIC_SEQ_CST); \
        case 2: return op((uint16_t *)p, data, __ATOMIC_SEQ_CST); \
        case 4: return op((uint32_t *)p, data, __ATOMIC_SEQ_CST); \
        case 8: return op((uint64_t *)p, data, __ATOMIC_SEQ_CST); \
        default: PANIC("Invalid length=" FMT_VARU64, length); return -1; \
    }

#define AMO_FUNCS(name, op) \
    switch (length) { \
        case 1: return op(( int8_t *)p, ( int8_t)data, __ATOMIC_SEQ_CST); \
        case 2: return op((int16_t *)p, (int16_t)data, __ATOMIC_SEQ_CST); \
        case 4: return op((int32_t *)p, (int32_t)data, __ATOMIC_SEQ_CST); \
        case 8: return op((int64_t *)p, (int64_t)data, __ATOMIC_SEQ_CST); \
        default: PANIC("Invalid length=" FMT_VARU64, length); return -1; \
    }

word_t Bus::do_atomic(word_t addr, word_t data, word_t length, AMO amo, bool &valid) {
    auto mem = match_memory(addr, length);
    if (mem != nullptr) {
        word_t offset = addr - mem->start;
        void *p = mem->data + offset;
        switch (amo) {
            case AMO_SWAP: AMO_FUNC (swap, __atomic_exchange_n);
            case AMO_ADD:  AMO_FUNC (add,  __atomic_fetch_add );
            case AMO_AND:  AMO_FUNC (and,  __atomic_fetch_and );
            case AMO_OR:   AMO_FUNC (or,   __atomic_fetch_or  );
            case AMO_XOR:  AMO_FUNC (xor,  __atomic_fetch_xor );
            case AMO_MIN:  AMO_FUNCS(min,  __atomic_fetch_min );
            case AMO_MAX:  AMO_FUNCS(max,  __atomic_fetch_max );
            case AMO_MINU: AMO_FUNC (min,  __atomic_fetch_min );
            case AMO_MAXU: AMO_FUNC (max,  __atomic_fetch_max );
            default: PANIC("Invalid AMO=%d", amo); return -1;
        }
        valid = true;
        return data;
    }

    auto map = this->match_mmio(addr, length);
    if (map == nullptr) {
        valid = false;
        return -1;
    }
    return map->dev->do_atomic(addr - map->start, data, length, amo, valid);
}

bool Bus::load_from_stream(std::istream &stream, word_t addr) {
    uint8_t *dest = (uint8_t *)this->get_ptr(addr);
    if (dest == nullptr) {
        WARN("Unable to write to destination.");
        return false;
    }

    word_t maxLength = this->get_ptr_length(addr);

    char buffer[BUFFER_SIZE];
    word_t writen = 0;

    std::streamsize count = BUFFER_SIZE;
    while (count == BUFFER_SIZE) {
        stream.read((char *)buffer, BUFFER_SIZE);
        count = stream.gcount();
        if (writen + count >= maxLength) {
            auto left = maxLength - writen;
            std::memcpy(dest, buffer, left);
            WARN("load image file to memory size out of range, only write %ld bytes", writen + count);
            break;
        }
        std::memcpy(dest, buffer, count);
        dest += count;
        writen += count;
    }
    return true;
}

bool Bus::load_from_stream(std::istream &stream, word_t addr, word_t length) {
    uint8_t *dest = (uint8_t *)this->get_ptr(addr);
    if (dest == nullptr) {
        WARN("Unable to write to destination.");
        return false;
    }
    
    word_t maxLength = this->get_ptr_length(addr);
    if (length > maxLength) {
        WARN("Length out of range.");
    }

    uint8_t buffer[BUFFER_SIZE];
    word_t writen = 0;
    while (true) {
        std::streamsize readLength = std::min(BUFFER_SIZE, length - writen);
        stream.read((char *)buffer, readLength);
        std::streamsize count = stream.gcount();
        if (count != readLength) {
            WARN("caught stream eof, only write " FMT_VARU64 " bytes", writen);
            break;
        }
        std::memcpy(dest, buffer, count);
        dest += count;
        writen += count;
        if (writen == length) {
            break;
        }
    }
    return true;
}

bool Bus::load_from_memory(void *src, word_t addr, word_t length) {
    void *dest = this->get_ptr(addr);
    if (dest == nullptr) {
        WARN("Unable to write to destination.");
        return false;
    }

    word_t leftLength = this->get_ptr_length(addr);
    if (length > leftLength) {
        WARN("Length out of range.");
        return 0;
    }
    
    std::memcpy(dest, src, length);
    
    return true;
}

bool Bus::dump(std::ostream &stream, word_t addr, word_t length) const {
    const void *src = this->get_ptr(addr);
    if (src == nullptr) {
        WARN("Unable to read from source.");
        return false;
    }

    word_t maxLength = this->get_ptr_length(addr);
    if (length > maxLength) {
        WARN("Lenght out of range.");
        return false;
    }

    stream.write((const char *)src, length);
    
    return true;
}

bool Bus::memset(word_t addr, word_t length, uint8_t byte) {
    void *dest = this->get_ptr(addr);
    word_t leftLength = this->get_ptr_length(addr);
    
    if (dest == nullptr || length > leftLength) {
        WARN("memset addr=" FMT_HEX64 " length=" FMT_VARU64 " out of range", addr, length);
        return false;
    }

    std::memset(dest, byte, length);

    return true;
}

bool Bus::memcpy(word_t addr, word_t length, void *dest) {
    if (dest == nullptr) {
        WARN("memcpy dest is nullptr.");
        return false;
    }

    void *src = this->get_ptr(addr);
    word_t leftLength = this->get_ptr_length(addr);
    
    if (src == nullptr || length > leftLength) {
        WARN("memcpy addr=" FMT_HEX64 " length=" FMT_VARU64 " out of range", addr, length);
        return false;
    }

    std::memcpy(dest, src, length);

    return true;
}

void *Bus::get_ptr(word_t addr) const{
    auto mem = match_memory(addr);
    if (mem != nullptr) {
        return mem->data + (addr - mem->start);
    }

    auto map = match_mmio(addr);
    if (map != nullptr) {
        return map->dev->get_ptr(addr - map->start);
    }

    return nullptr;
}

void *Bus::get_ptr(word_t addr, word_t length) const {
    auto mem = match_memory(addr);
    if (mem != nullptr) {
        return mem->data + (addr - mem->start);
    }
    return nullptr;
}

word_t Bus::get_ptr_length(word_t addr) const {
    auto mem = match_memory(addr);
    if (mem != nullptr) {
        return mem->end - addr;
    }

    auto map = match_mmio(addr);
    if (map != nullptr) {
        return map->size - (addr - map->start);
    }
    return 0;
}

std::mutex *Bus::get_lock() {
    return &this->lock;
}

Bus::~Bus() {
    for (auto &m : memoryMaps) {
        ::operator delete[](m->data, std::align_val_t(8));
        delete m;
    }
    memoryMaps.clear();
    for (auto &m : mmioMaps) {
        delete m;
    }
}
