#include "device/virtio/virtio.hpp"
#include "device/def.hpp"
#include "device/virtio/def.h"
#include "log.h"
#include "word.h"
#include "debug.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

static inline void set_upper(uint64_t &value, uint32_t upper) {
    value = (value & 0xffffffffULL) | ((uint64_t)upper << 32);
}

static inline void set_lower(uint64_t &value, uint32_t lower) {
    value =  (value & ~0xffffffffULL) | (lower & 0xffffffff);
}

using namespace kxemu::device;

VirtIO::VirtIO(
    uint32_t deviceID, 
    unsigned int featuresNumMax, 
    unsigned int queueCount
) 
    : deviceID(deviceID), featuresNumMax(featuresNumMax), queueCount(queueCount) {
    this->virtQueues = new VirtQueue[queueCount];
}

VirtIO::~VirtIO() {
    delete[] this->virtQueues;
}

void VirtIO::reset() {
    this->state = IDLE;
    if (this->interruptStatus != 0) {
        this->interruptStatus = 0;
        this->set_irq_level(false);
    }
    for (unsigned int i = 0; i < this->queueCount; i++) {
        this->virtQueues[i].lastAvailIndex = 0;
    }
}

word_t VirtIO::read_status() {
    word_t r  = 0;
    switch (this->state) {
        case DRIVER_OK:   r |= VIRTIO_STATUS_DRIVER_OK;   [[fallthrough]];
        case FEATURES_OK: r |= VIRTIO_STATUS_FEATURES_OK; [[fallthrough]];
        case DRIVERED:    r |= VIRTIO_STATUS_DRIVER;      [[fallthrough]];
        case ACKNOWLEGED: r |= VIRTIO_STATUS_ACKNOWLEDGE; [[fallthrough]];
        case IDLE: break;
    }
    return r;
}

void VirtIO::write_status(word_t data) {
    if (data == 0) {
        this->reset();
        return ;
    }
    
    switch (this->state) {
        case IDLE: 
            if (data & VIRTIO_STATUS_ACKNOWLEDGE) this->state = ACKNOWLEGED;
            else break;
        case ACKNOWLEGED:
            if (data & VIRTIO_STATUS_DRIVER) this->state = DRIVERED;
            else break;
        case DRIVERED:
            if (data & VIRTIO_STATUS_FEATURES_OK) this->state = FEATURES_OK;
            else break;
        case FEATURES_OK:
            if (data & VIRTIO_STATUS_DRIVER_OK) this->state = DRIVER_OK;
            else break;
        case DRIVER_OK:
            break;
    }
}

void VirtIO::notify_queue(uint32_t queueIdx) {
    if (queueIdx >= this->queueCount) return ;
    // A notification racing with a device reset is dropped with the reset.
    if (this->resetting) return;

    VirtQueue &queue = this->virtQueues[queueIdx];
    if (queue.queueNum == 0) return;
    
    // struct virtq_avail {
    //     #define VIRTQ_AVAIL_F_NO_INTERRUPT 1
    //     le16 flags;
    //     le16 idx;
    //     le16 ring[ /* Queue Size */ ];
    //     le16 used_event; /* Only if VIRTIO_F_EVENT_IDX */
    // };
    
    char *avail = (char *)this->bus->get_ptr(queue.p_avail, (3 + queue.queueNum) * 2);
    if (avail == nullptr) {
        WARN("Failed to get struct virtq_avail");
        return ;
    }
    
    uint16_t availIndex = *(uint16_t *)(avail + 2);
    uint16_t *ring = (uint16_t *)(avail + 4);
    
    VirtQueueDescriptor *descriptors = (VirtQueueDescriptor *)this->bus->get_ptr(queue.p_desc, sizeof(VirtQueueDescriptor) * queue.queueNum);
    if (descriptors == nullptr) {
        WARN("Failed to get struct virtq_desc");
        return ;
    }

    while (queue.lastAvailIndex != availIndex) {
        uint16_t current = queue.lastAvailIndex % queue.queueNum;
        queue.lastAvailIndex++;
        
        Request req{queueIdx, ring[current], {}};
        bool valid = req.head < queue.queueNum;
        uint16_t descIdx = req.head;
        // A chain longer than the queue must contain a loop.
        for (unsigned int i = 0; valid; i++) {
            const VirtQueueDescriptor &descriptor = descriptors[descIdx];
            if (i >= queue.queueNum || (descriptor.flags & VIRTQ_DESC_F_INDIRECT)) {
                valid = false;
                break;
            }
            uint8_t *ptr = (uint8_t *)this->bus->get_ptr(descriptor.addr, descriptor.len);
            if (ptr == nullptr && descriptor.len != 0) {
                valid = false;
                break;
            }
            req.buffers.push_back({ptr, descriptor.len, (descriptor.flags & VIRTQ_DESC_F_WRITE) != 0});
            if (!(descriptor.flags & VIRTQ_DESC_F_NEXT)) {
                break;
            }
            descIdx = descriptor.next;
            valid = descIdx < queue.queueNum;
        }

        if (!valid) {
            WARN("VirtIO: invalid descriptor chain, head=%u", (unsigned)req.head);
            // Return the chain unused so that the driver does not wait forever.
            this->virtio_complete_locked(req, 0);
            continue;
        }
        this->inflight++;
        this->virtio_submit(std::move(req));
    }
}

void VirtIO::virtio_complete(const Request &req, uint32_t len) {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->virtio_complete_locked(req, len);
    this->inflight--;
    if (this->inflight == 0) {
        this->idle.notify_all();
    }
}

void VirtIO::virtio_complete_locked(const Request &req, uint32_t len) {
    const VirtQueue &queue = this->virtQueues[req.queueIndex];
    
    // struct virtq_used {
    //     #define VIRTQ_USED_F_NO_NOTIFY 1
    //     le16 flags;
    //     le16 idx;
    //     struct virtq_used_elem ring[ /* Queue Size */];
    //     le16 avail_event; /* Only if VIRTIO_F_EVENT_IDX */
    // };

    // struct virtq_used_elem {
    //     /* Index of start of used descriptor chain. */
    //     le32 id;
    //     /* Total length of the descriptor chain which was used (written to) */
    //     le32 len;
    // };
    
    char *used = (char *)this->bus->get_ptr(queue.p_used, 6 + sizeof(VirtqUsedElem) * queue.queueNum);
    if (used == nullptr) {
        WARN("Failed to get struct virtq_used");
        return;
    }
    bool noNotify = (*(uint16_t *)used) & VIRTQ_USED_F_NO_NOTIFY;
    
    std::atomic_ref<uint16_t> usedIdx(*(uint16_t *)(used + 2));
    const uint16_t idx = usedIdx.load(std::memory_order_relaxed);
    VirtqUsedElem *ring = (VirtqUsedElem *)(used + 4);
    ring[idx % queue.queueNum].id  = req.head;
    ring[idx % queue.queueNum].len = len;
    // Publish the data and the element before a polling driver can see the index.
    usedIdx.store(idx + 1, std::memory_order_release);

    if (!noNotify) {
        this->interruptStatus |= VIRTIO_INT_USED_BUFFER;
        this->set_irq_level(true);
    }
}

void VirtIO::wait_idle(std::unique_lock<std::mutex> &lock) {
    this->idle.wait(lock, [this] { return this->inflight == 0; });
}

void VirtIO::quiesce() {
    std::unique_lock<std::mutex> lock(this->mtx);
    this->wait_idle(lock);
}

uint32_t VirtIO::read_device_features() {
    if (this->state != DRIVERED) {
        return 0;
    }
    
    unsigned int start = this->deviceFeaturesSelect * 32;
    if (start >= featuresNumMax) {
        return 0;
    }
   
    unsigned int end = start + 32;
    if (end > this->featuresNumMax) {
        end = this->featuresNumMax;
    }
    
    uint32_t features = 0;
    for (unsigned int i = start; i < end; i++) {
        features |= this->get_device_features_bit(i) << (i - start);
    }

    return features;
}

void VirtIO::write_driver_features(uint32_t features) {
    if (this->state != DRIVERED) {
        return;
    }

    unsigned int start = this->driverFeaturesSelect * 32;
    if (start >= this->featuresNumMax) {
        return;
    }

    unsigned int end = start + 32;
    if (end > this->featuresNumMax) {
        end = this->featuresNumMax;
    }
    
    for (unsigned int i = start; i < end; i++) {
        this->set_driver_features_bit(i, features & (1 << (i - start)));
    }
}

word_t VirtIO::read(word_t offset, word_t size, bool &valid) {
    std::lock_guard<std::mutex> lock(this->mtx);
    // Configuration Map
    if (offset >= 0x100 && offset + size <= 0x100 + this->sizeof_configuration) {
        offset -= 0x100;
        valid = true;
        void *p = (char *)this->configuration + offset;
        switch (size) {
            case 1: return *(uint8_t  *)p;
            case 2: return *(uint16_t *)p;
            case 4: return *(uint32_t *)p;
            case 8: return *(uint64_t *)p;
            default: PANIC("Invalid read size=" FMT_VARU64, size);
        }
    }
    
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
        
        case 0x30: return this->queueSelect;
        case 0x34: return this->queueNumMax;
        case 0x44: return this->virtQueues[this->queueSelect].ready;

        case 0x60: return this->interruptStatus; // No configuration change interrupt yet
        case 0x70: return this->read_status();
    }

    valid = false;
    return -1;
}

bool VirtIO::write(word_t offset, word_t data, word_t size) {
    std::unique_lock<std::mutex> lock(this->mtx);
    // Configuration Map
    if (offset >= 0x100 && offset + size <= 0x100 + this->sizeof_configuration) {
        if (this->sizeof_configuration == 0) {
            WARN("VirtIO: configuration size is 0\n");
            return false;
        }

        offset -= 0x100;
        char *c = new char[this->sizeof_configuration];
        void *p = c + offset;
        
        switch (size) {
            case 1: *(uint8_t  *)p = data; break;
            case 2: *(uint16_t *)p = data; break;
            case 4: *(uint32_t *)p = data; break;
            case 8: *(uint64_t *)p = data; break;
            default: PANIC("Invalid read size=" FMT_VARU64, size);
        }

        this->update_configuration(c);

        delete []c;

        return true;
    }

    if (size != 4) {
        WARN("VirtIO: write size " FMT_VARU64 " not supported\n", size);
        return false;
    }

    bool valid = true;
    switch (offset) {
        case 0x14: this->deviceFeaturesSelect = data; break;
        case 0x20: this->write_driver_features(data); break;
        case 0x24: this->driverFeaturesSelect = data; break;
        
        case 0x30: this->queueSelect = data >= this->queueCount ? this->queueSelect : data; break;
        case 0x38: this->virtQueues[this->queueSelect].queueNum = data; break;
        case 0x44: this->virtQueues[this->queueSelect].ready = data == 1; break;

        case 0x50: this->notify_queue(data); break;
        
        case 0x64: this->ack_interrupt(data); break;
        case 0x70:
            if (data == 0) {
                // Device reset: in-flight requests still write guest memory and
                // the used ring, so let them finish first. wait_idle() drops mtx,
                // so hold off notifications from other harts meanwhile.
                this->resetting = true;
                this->wait_idle(lock);
                this->resetting = false;
            }
            this->write_status(data);
            break;

        case 0x80: set_lower(this->virtQueues[this->queueSelect].p_desc , data); break;
        case 0x84: set_upper(this->virtQueues[this->queueSelect].p_desc , data); break;
        case 0x90: set_lower(this->virtQueues[this->queueSelect].p_avail, data); break;
        case 0x94: set_upper(this->virtQueues[this->queueSelect].p_avail, data); break;
        case 0xa0: set_lower(this->virtQueues[this->queueSelect].p_used , data); break;
        case 0xa4: set_upper(this->virtQueues[this->queueSelect].p_used , data); break;

        default: valid = false; break;
    }

    return valid;
}

void VirtIO::connect_to_bus(Bus *bus) {
    this->bus = bus;
}

void VirtIO::ack_interrupt(word_t data) {
    if (this->interruptStatus == 0) return;
    this->interruptStatus &= ~data;
    if (this->interruptStatus == 0) {
        this->set_irq_level(false);
    }
}
