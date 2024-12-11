#include "kdb/config.h"
#include "kdb/kdb.h"
#include "memory/map.h"
#include "memory/memory.h"
#include "test_img.h"
#include <cstdint>
#include <cstring>

#define TEST_IMG test_img_athrimatic

Memory *kdb::memory = nullptr;

void kdb::init_memory() {
    memory = new Memory();
    // StorageMemoryMap *mem = new StorageMemoryMap(MEM_SIZE);
    // memory->add_memory_map("mem", MEM_BASE, MEM_SIZE, mem);
    // memory->load_from_memory((uint8_t *)test_img_athrimatic, 0x80000000, sizeof(test_img_athrimatic));
}

void kdb::deinit_memory() {
    memory->free_all();
    delete memory;
}
