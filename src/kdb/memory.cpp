#include "kdb/kdb.h"
#include "memory/memory.h"

Memory *kdb::memory = nullptr;

void kdb::init_memory() {
    memory = new Memory();
}

void kdb::deinit_memory() {
    memory->free_all();
    delete memory;
}
