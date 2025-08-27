#include "device/mmio.hpp"
#include "kdb/kdb.hpp"
#include "device/bus.hpp"

#include <vector>

using namespace kxemu;

device::Bus *kdb::bus = nullptr;

std::vector<device::MMIODev *> deviceList;

void kdb::device::init() {
    bus = new kxemu::device::Bus();
}

void kdb::device::add(kxemu::device::MMIODev *dev) {
    deviceList.push_back(dev);
}

void kdb::device::deinit() {
    for (auto dev : deviceList) {
        delete dev;
    }
    delete bus;
}
