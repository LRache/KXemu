#include "kdb/config.h"
#include "kdb/kdb.h"
#include "memory/map.h"
#include "memory/memory.h"
#include "test_img.h"
#include <cstring>

#define TEST_IMG test_img_branch

Memory *kdb::memory = nullptr;

void kdb::init_memory() {
    memory = new Memory();
    StorageMemoryMap *mem = new StorageMemoryMap(MEM_SIZE);
    memory->add_memory_map("mem", MEM_BASE, MEM_SIZE, mem);

    std::memcpy(mem->data, TEST_IMG, sizeof(TEST_IMG));
}

void kdb::deinit_memory() {
    memory->free_all();
    delete memory;
}
