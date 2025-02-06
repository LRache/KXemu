#include "kdb/kdb.h"
#include "device/bus.h"

using namespace kxemu;

device::Bus *kdb::bus = nullptr;

void kdb::init_bus() {
    bus = new device::Bus();
}

void kdb::deinit_bus() {
    bus->free_all();
    delete bus;
}
