#include "cpu/riscv/plic.h"
#include "device/bus.h"
#include "device/def.h"

using namespace kxemu::device;

void PLIC::reset() {

}

word_t PLIC::read(word_t offset, word_t size, bool &valid) {
    valid = false;
    return 0;
}

bool PLIC::write(word_t offset, word_t data, word_t size) {
    return false;
}

void PLIC::connect_to_bus(Bus *bus) {
    this->bus = bus;
}

bool PLIC::scan_interrupt() {
    for (auto &dev : this->bus->mmioMaps) {
        if (dev->map->has_interrupt()) {
            return true;
        }
    }
    return false;
}
