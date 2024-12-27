#include "device/memory.h"
#include "log.h"

StorageMemoryMap::StorageMemoryMap(uint64_t length) {
    this->length = length;
    data = new uint8_t[length];
}

StorageMemoryMap::~StorageMemoryMap() {
    delete[] data;
}

word_t StorageMemoryMap::read(word_t offset, int size) {
    uint64_t ret = 0;
    switch (size) {
        case 1: ret = data[offset]; break;
        case 2: ret = *(uint16_t *)(data + offset); break;
        case 4: ret = *(uint32_t *)(data + offset); break;
        #ifdef ISA_64
        case 8: ret = *(uint64_t *)(data + offset); break;
        #endif
        default: PANIC("Invalid size=%d", size); return -1;
    }
    return ret;
}

bool StorageMemoryMap::write(word_t offset, word_t data, int size) {
    switch (size) {
        case 1: *(uint8_t  *)(this->data + offset) = (uint8_t )data; break;
        case 2: *(uint16_t *)(this->data + offset) = (uint16_t)data; break;
        case 4: *(uint32_t *)(this->data + offset) = (uint32_t)data; break;
        #ifdef ISA_64
        case 8: *(uint64_t *)(this->data + offset) = (uint64_t)data; break;
        #endif
        default: PANIC("Invalid size=%d", size); return false;
    }
    return true;
}

uint8_t *StorageMemoryMap::get_ptr(word_t offset) {
    return data + offset;
}

std::string StorageMemoryMap::get_type_str() const {
    return "storage";
}
