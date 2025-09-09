#ifndef __KXEMU_DEVICE_VIRTIO_VIRTIO_HPP__
#define __KXEMU_DEVICE_VIRTIO_VIRTIO_HPP__

#include "device/bus.hpp"
#include "device/def.hpp"
#include <cstdint>
#include <mutex>
#include <vector>

namespace kxemu::device {

// This is an implementation of the VirtIO device.
// See https://docs.oasis-open.org/virtio/virtio/v1.2/virtio-v1.2.pdf
// for the specification.

class VirtIO : public MMIODev {
protected:
    Bus *bus;

    enum State {
        IDLE,
        ACKNOWLEGED,
        DRIVERED,
        FEATURES_OK,
        DRIVER_OK,
    } state;
    
    // MMIO Device Register
    uint32_t deviceID; // 0x008 Device ID, used to identify the device type
    uint32_t deviceFeaturesSelect; // 0x010
    uint32_t driverFeaturesSelect; // 0x014
    uint32_t queueSelect; // 0x030
    static constexpr uint32_t queueNumMax = 12;
    /* 0x010 Device features */
    uint32_t read_device_features();
    void write_driver_features(uint32_t features);
    /* 0x070 Device status field */ 
    word_t read_status();
    void  write_status(word_t data);
    /* 0x050 Queue Notify */
    void notify_queue(uint32_t idx);
    /* 0x100+ configuration space */
    void *configuration = nullptr;
    word_t sizeof_configuration = 0;
    virtual bool update_configuration(const void *newConfig) { return true; };

    // Device Features
    unsigned int featuresNumMax;
    virtual bool get_device_features_bit(unsigned int bit) { return false; }
    virtual void set_driver_features_bit(unsigned int bit, bool value) {}

    // Virtqueue
    struct VirtQueueDescriptor {
        uint64_t addr;
        uint32_t len;
        uint16_t flags;
        uint16_t next;
    };

    // struct virtq_avail {
    //    uint16_t flags;
    //    uint16_t idx;
    //    uint16_t ring[ /* Queue Size */ ];
    //    uint16_t used_event; /* Only if VIRTIO_F_EVENT_IDX */
    // };

    // struct virtq_used_elem {
    //     /* Index of start of used descriptor chain. */
    //     le32 id;
    //     /* Total length of the descriptor chain which was used (written to) */
    //     le32 len;
    // };

    struct VirtqUsedElem {
        uint32_t id;
        uint32_t len;
    };

    unsigned int queueCount = 0;
    struct VirtQueue {
        uint64_t p_desc  = 0;  // 0x80, 0x84 Pointer to Descriptor Table
        uint64_t p_avail = 0;  // 0x90, 0x94 Pointer to Available Ring
        uint64_t p_used  = 0;  // 0xa0, 0xa4 Pointer to Used Ring
        uint32_t queueNum = 12;
        bool ready = false;
        uint16_t lastAvailIndex = 0;
    };
    VirtQueue *virtQueues;

    struct Buffer {
        word_t addr;
        word_t len;
        bool write;
    };

    virtual bool virtio_handle_req(const std::vector<Buffer> &buffer, uint32_t &len) = 0;

    void virtio_handle_done(uint32_t len, unsigned int queueIndex, unsigned int descIndex);

    bool interrupt = false;
    bool interrupt_pending() override;
    void clear_interrupt() override;

    std::mutex mtx;

public:
    VirtIO(uint32_t deviceID, unsigned int featuresNumMax, unsigned int queueCount);
    virtual ~VirtIO();

    void reset() override;
    word_t read(word_t offset, word_t size, bool &valid) override;
    bool write(word_t offset, word_t data, word_t size) override;
    void connect_to_bus(Bus *bus) override;
};

} // namespace kxemu::device

#endif
