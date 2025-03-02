#ifndef __KXEMU_DEVICE_VIRTIO_VIRTIO_H__
#define __KXEMU_DEVICE_VIRTIO_VIRTIO_H__

#include "device/bus.h"
#include <cstdint>

namespace kxemu::device {

// This is an implementation of the VirtIO device.
// See https://docs.oasis-open.org/virtio/virtio/v1.2/virtio-v1.2.pdf
// for the specification.

class VirtIO : public MMIOMap {
private:
    // MMIO Device Register
    uint32_t deviceID; // 0x008 Device ID, used to identify the device type
    uint32_t deviceFeaturesSelect; // 0x010
    uint32_t driverFeaturesSelect; // 0x014
    uint32_t queueSelect; // 0x030
    uint32_t queueNumMax; // 0x034
    uint32_t status;   // 0x070 Device status field

    uint64_t queueDesc;   // Queue descriptor address
    uint64_t queueDriver; // Queue driver(avail) address
    uint64_t queueDevice; // Queue device(used) address

    // Device Features
    unsigned int featuresCount;
    virtual bool get_device_features_bit(unsigned int bit) = 0;
    virtual void set_driver_features_bit(unsigned int bit, bool value) = 0;
    uint32_t read_device_features();
    void write_driver_features(uint32_t features);

    // Virtqueue
    struct VirtQueue {

    };

public:
    VirtIO(uint32_t deviceID, unsigned int featuresCount);

    void reset() override;
    word_t read(word_t offset, word_t size, bool &valid) override;
    bool write(word_t offset, word_t data, word_t size) override;
};

} // namespace kxemu::device

#endif
