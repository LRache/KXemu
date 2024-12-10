#include "memory/map.h"
#include "common.h"
#include <cstdint>

StorageMemoryMap::StorageMemoryMap(uint64_t length) {
    this->length = length;
    data = new uint8_t[length];
}

StorageMemoryMap::~StorageMemoryMap() {
    delete[] data;
}

word_t StorageMemoryMap::read(word_t offset, int size) const {
    uint64_t ret = 0;
    switch (size) {
        case 1: ret = data[offset]; break;
        case 2: ret = *(uint16_t *)(data + offset); break;
        case 4: ret = *(uint32_t *)(data + offset); break;
        case 8: ret = *(uint64_t *)(data + offset); break;
        default: break;
    }
    return ret;
}

bool StorageMemoryMap::write(word_t offset, word_t data, int size) {
    switch (size) {
        case 1: *(uint8_t  *)(this->data + offset) = (uint8_t )data; break;
        case 2: *(uint16_t *)(this->data + offset) = (uint16_t)data; break;
        case 4: *(uint32_t *)(this->data + offset) = (uint32_t)data; break;
        case 8: *(uint64_t *)(this->data + offset) = (uint64_t)data; break;
        default: return false;
    }
    return true;
}

uint8_t *StorageMemoryMap::get_ptr(word_t offset) {
    return data + offset;
}
