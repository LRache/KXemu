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
        if (m->start <= addr && addr + length <= m->start + m->size) {
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

bool Bus::add_memory_map(const std::string &name, word_t start, word_t size) {
    // check if overlap
    for (auto &m : memoryMaps) {
        if (m->start <= start && start < m->start + m->size) {
            return false;
        }
        if (start <= m->start && m->start < start + size) {
            return false;
        }
    }
    
    auto m = new MemoryBlock;
    m->name = name;
    m->start = start;
    m->size = size;
    m->data = new((std::align_val_t)8) uint8_t[size];
    memoryMaps.push_back(m);
    
    return true;
}

bool Bus::add_mmio_map(const std::string &name, word_t start, word_t length, MMIOMap *map) {
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
    m->name = name;
    m->start = start;
    m->size = length;
    m->map = map;
    mmioMaps.push_back(m);
    return true;
}

void Bus::free_all() {
    for (auto &m : memoryMaps) {
        delete m->data;
        delete m;
    }
    memoryMaps.clear();
    for (auto &m : mmioMaps) {
        delete m;
    }
    mmioMaps.clear();
}

word_t Bus::read(word_t addr, word_t length, bool &valid) const {
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
        valid = true;
        return data;
    }

    auto map = this->match_mmio(addr, length);
    if (likely(map != nullptr)) {
        return map->map->read(addr - map->start, length, valid);
    }

    valid = false;
    // WARN("read addr: 0x%" PRIx64 ", size: " FMT_VARU64 ", out of range", addr, length);
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
        return map->map->write(addr - map->start, data, length);
    }
    return false;
}

void Bus::update() {
    for (auto &m : mmioMaps) {
        m->map->update();
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
    return map->map->do_atomic(addr - map->start, data, length, amo, valid);
}

bool Bus::load_from_stream(std::istream &stream, word_t addr) {
    uint8_t *dest = this->get_ptr(addr);
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
    uint8_t *dest = this->get_ptr(addr);
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
    uint8_t *dest = this->get_ptr(addr);
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
    const uint8_t *src = this->get_ptr(addr);
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
    uint8_t *dest = this->get_ptr(addr);
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

    uint8_t *src = this->get_ptr(addr);
    word_t leftLength = this->get_ptr_length(addr);
    
    if (src == nullptr || length > leftLength) {
        WARN("memcpy addr=" FMT_HEX64 " length=" FMT_VARU64 " out of range", addr, length);
        return false;
    }

    std::memcpy(dest, src, length);

    return true;
}

uint8_t *Bus::get_ptr(word_t addr) const{
    auto mem = match_memory(addr);
    if (mem != nullptr) {
        return mem->data + (addr - mem->start);
    }

    auto map = match_mmio(addr);
    if (map != nullptr) {
        return map->map->get_ptr(addr - map->start);
    }

    return nullptr;
}

word_t Bus::get_ptr_length(word_t addr) const {
    auto mem = match_memory(addr);
    if (mem != nullptr) {
        return mem->size - (addr - mem->start);
    }

    auto map = match_mmio(addr);
    if (map != nullptr) {
        return map->size - (addr - map->start);
    }
    return 0;
}

Bus::~Bus() {
    for (auto &m : memoryMaps) {
        delete m;
    }
    memoryMaps.clear();
}
