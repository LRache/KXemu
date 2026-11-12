#ifndef __KXEMU_DEVICE_VIRTIO_VIRTIO_HPP__
#define __KXEMU_DEVICE_VIRTIO_VIRTIO_HPP__

#include "device/bus.hpp"
#include "device/def.hpp"
#include <condition_variable>
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
    // The VirtIO spec requires a power of two; 256 matches QEMU's virtio-mmio.
    static constexpr uint32_t queueNumMax = 256;
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

    // A descriptor chain, resolved to host pointers into guest memory.
    struct Buffer {
        uint8_t *ptr;
        uint32_t len;
        bool write; // Device-writable
    };
    struct Request {
        unsigned int queueIndex;
        uint16_t head; // Index of the first descriptor, returned in the used ring
        std::vector<Buffer> buffers;
    };

    // Called on the hart with mtx held. The device may finish the request later
    // on any thread, and must then call virtio_complete() exactly once.
    virtual void virtio_submit(Request &&req) = 0;
    // Thread-safe. len is the number of bytes written into the chain.
    void virtio_complete(const Request &req, uint32_t len);
    void virtio_complete_locked(const Request &req, uint32_t len);

    // InterruptStatus bits; the IRQ line is asserted while any is set.
    static constexpr uint32_t VIRTIO_INT_USED_BUFFER = 1 << 0;
    uint32_t interruptStatus = 0;
    void ack_interrupt(word_t data);

    // Guards the registers, the queues and the used rings. The IRQ is driven with
    // it held, so the order is VirtIO, then the interrupt controller.
    std::mutex mtx;
    // Requests submitted and not yet completed, guarded by mtx.
    unsigned int inflight = 0;
    // A device reset is waiting for in-flight requests; guarded by mtx.
    bool resetting = false;
    std::condition_variable idle;
    void wait_idle(std::unique_lock<std::mutex> &lock);

public:
    VirtIO(uint32_t deviceID, unsigned int featuresNumMax, unsigned int queueCount);
    virtual ~VirtIO();

    void reset() override;
    word_t read(word_t offset, word_t size, bool &valid) override;
    bool write(word_t offset, word_t data, word_t size) override;
    void connect_to_bus(Bus *bus) override;
    // Wait for in-flight requests; they may still raise the IRQ until then.
    void quiesce() override;
};

} // namespace kxemu::device

#endif
