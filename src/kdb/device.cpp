#include "device/mmio.hpp"
#include "kdb/kdb.hpp"
#include "device/bus.hpp"
#include "utils/io-loop.hpp"

#include <vector>

using namespace kxemu;

device::Bus *kdb::bus = nullptr;

std::vector<device::MMIODev *> deviceList;

static utils::IOLoop ioLoop;

void kdb::device::init() {
    bus = new kxemu::device::Bus();
    ioLoop.start();
}

utils::IOLoop &kdb::device::io_loop() {
    return ioLoop;
}

void kdb::device::stop() {
    ioLoop.stop();
    for (auto dev : deviceList) {
        dev->quiesce();
    }
}

void kdb::device::add(kxemu::device::MMIODev *dev) {
    deviceList.push_back(dev);
}

void kdb::device::deinit() {
    stop(); // Before freeing the devices.
    for (auto dev : deviceList) {
        delete dev;
    }
    delete bus;
}
