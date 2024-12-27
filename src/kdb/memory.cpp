#include "kdb/kdb.h"
#include "device/bus.h"

Bus *kdb::bus = nullptr;

void kdb::init_bus() {
    bus = new Bus();
}

void kdb::deinit_bus() {
    bus->free_all();
    delete bus;
}
