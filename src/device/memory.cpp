#include "device/memory.h"
#include "device/bus.h"
#include "log.h"
#include <cstdint>

using namespace kxemu::device;

Memory::Memory(uint64_t length) {
    this->length = length;
    data = new uint8_t[length];
}

Memory::~Memory() {
    delete[] data;
}

word_t Memory::read(word_t offset, int size) {
    switch (size) {
        case 1: return *(uint8_t  *)(data + offset);
        case 2: return *(uint16_t *)(data + offset);
        case 4: return *(uint32_t *)(data + offset);
        case 8: return *(uint64_t *)(data + offset);
        default: PANIC("Invalid size=%d", size); return -1;
    }
}

bool Memory::write(word_t offset, word_t data, int size) {
    switch (size) {
        case 1: *(uint8_t  *)(this->data + offset) = (uint8_t )data; break;
        case 2: *(uint16_t *)(this->data + offset) = (uint16_t)data; break;
        case 4: *(uint32_t *)(this->data + offset) = (uint32_t)data; break;
        case 8: *(uint64_t *)(this->data + offset) = (uint64_t)data; break;
        default: PANIC("Invalid size=%d", size); return false;
    }
    return true;
}

uint8_t *Memory::get_ptr(word_t offset) {
    return data + offset;
}

const char *Memory::get_type_name() const {
    return "memory";
}

#define AMO_FUNC(name, op) \
    switch (size) { \
        case 1: return op(( uint8_t *)p, data, __ATOMIC_SEQ_CST); \
        case 2: return op((uint16_t *)p, data, __ATOMIC_SEQ_CST); \
        case 4: return op((uint32_t *)p, data, __ATOMIC_SEQ_CST); \
        case 8: return op((uint64_t *)p, data, __ATOMIC_SEQ_CST); \
        default: valid = false; return -1; \
    } \

#define AMO_FUNCS(name, op) \
    switch (size) { \
        case 1: return op(( int8_t *)p, ( int8_t)data, __ATOMIC_SEQ_CST); \
        case 2: return op((int16_t *)p, (int16_t)data, __ATOMIC_SEQ_CST); \
        case 4: return op((int32_t *)p, (int32_t)data, __ATOMIC_SEQ_CST); \
        case 8: return op((int64_t *)p, (int64_t)data, __ATOMIC_SEQ_CST); \
        default: valid = false; return -1; \
    } \

word_t Memory::do_atomic(word_t offset, word_t data, int size, AMO amo, bool &valid) {
    void *p = this->data + offset;
    valid = true;
    switch (amo) {
        case AMO_SWAP: AMO_FUNC (swap, __atomic_exchange_n);
        case AMO_ADD:  AMO_FUNC (add,  __atomic_fetch_add );
        case AMO_AND:  AMO_FUNC (and,  __atomic_fetch_and );
        case AMO_OR:   AMO_FUNC (or,   __atomic_fetch_or  );
        case AMO_XOR:  AMO_FUNC (xor,  __atomic_fetch_xor );
        case AMO_MINU: AMO_FUNC (minu, __atomic_fetch_min );
        case AMO_MAXU: AMO_FUNC (maxu, __atomic_fetch_max );
        case AMO_MIN:  AMO_FUNCS(min,  __atomic_fetch_min );
        case AMO_MAX:  AMO_FUNCS(max,  __atomic_fetch_max );
        default: valid = false; return 0;
    }
}
