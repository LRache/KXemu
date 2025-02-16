#include "device/virtio/virtio.h"
#include "device/virtio/def.h"
#include "device/bus.h"
#include "log.h"
#include "word.h"

using namespace kxemu::device;

VirtIO::VirtIO(uint32_t deviceID, unsigned int featuresCount) 
    : deviceID(deviceID), featuresCount(featuresCount) {}

void VirtIO::reset() {
    this->status = 0;
}

uint32_t VirtIO::read_device_features() {
    unsigned int start = this->deviceFeaturesSelect * 32;
    unsigned int end = start + 32;
    if (end > this->featuresCount) {
        end = this->featuresCount;
    }
    uint32_t features = 0;
    for (unsigned int i = start; i < end; i++) {
        features |= this->get_device_features_bit(i) << (i - start);
    }

    return features;
}

void VirtIO::write_driver_features(uint32_t features) {
    unsigned int start = this->driverFeaturesSelect * 32;
    if (start >= this->featuresCount) {
        return;
    }

    unsigned int end = start + 32;
    if (end > this->featuresCount) {
        end = this->featuresCount;
    }
    for (unsigned int i = start; i < end; i++) {
        this->set_driver_features_bit(i, features & (1 << (i - start)));
    }
}

word_t VirtIO::read(word_t offset, word_t size, bool &valid) {
    if (size != 4) {
        WARN("VirtIO: read size" FMT_VARU64 "not supported\n", size);
        valid = false;
        return -1;
    }

    valid = true;
    switch (offset) {
        case 0x00: return VIRTIO_MAGIC;
        case 0x04: return VIRTIO_VERSION;
        case 0x08: return this->deviceID;
        case 0x0c: return VIRTIO_VENDOR;
        case 0x10: return this->read_device_features();
    }

    valid = false;
    return -1;
}

bool VirtIO::write(word_t offset, word_t data, word_t size) {
    if (size != 4) {
        WARN("VirtIO: write size " FMT_VARU64 " not supported\n", size);
        return false;
    }

    bool valid = true;
    switch (offset) {
        case 0x14: this->deviceFeaturesSelect = data; break;
        case 0x20: this->write_driver_features(data); break;
        case 0x24: this->driverFeaturesSelect = data; break;
        case 0x80: this->queueDesc &= ~0xffffffffULL; this->queueDesc |= data; break;
        case 0x84: this->queueDesc &= 0xffffffffULL; this->queueDesc |= data << 32; break;
        case 0x90: this->queueDriver &= ~0xffffffffULL; this->queueDriver |= data; break;
        case 0x94: this->queueDriver &= 0xffffffffULL; this->queueDriver |= data << 32; break;
        case 0xa0: this->queueDevice &= ~0xffffffffULL; this->queueDevice |= data; break;
        case 0xa4: this->queueDevice &=  0xffffffffULL; this->queueDevice |= data << 32; break;
        default: valid = false; break;
    }

    return valid;
}
