#include "device/memory.h"
#include "log.h"

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
