#include "device/virtio/virtio.h"
#include "debug.h"
#include "device/def.h"
#include "device/virtio/def.h"
#include "log.h"
#include "word.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#define SET_UPPER(target, upper) (target) &= ~0xffffffffULL; (target) |= (upper);
#define SET_LOWER(target, lower) (target) &=  0xffffffffULL; (target) |= (uint64_t)(lower) << 32;

using namespace kxemu::device;

VirtIO::VirtIO(
    uint32_t deviceID, 
    unsigned int featuresNumMax, 
    unsigned int queueCount
) 
    : deviceID(deviceID), featuresNumMax(featuresNumMax), queueCount(queueCount) {
    this->virtQueues = new VirtQueue[queueCount];
}

void VirtIO::reset() {
    this->state = IDLE;
}

word_t VirtIO::read_status() {
    word_t r  = 0;
    switch (this->state) {
        case DRIVER_OK:   r |= VIRTIO_STATUS_DRIVER_OK;
        case FEATURES_OK: r |= VIRTIO_STATUS_FEATURES_OK;
        case DRIVERED:    r |= VIRTIO_STATUS_DRIVER;
        case ACKNOWLEGED: r |= VIRTIO_STATUS_ACKNOWLEDGE;
        case IDLE: break;
    }
    return r;
}

void VirtIO::write_status(word_t data) {
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
    if (queueIdx > this->queueCount) return ;

    const VirtQueue &queue = this->virtQueues[queueIdx];
    word_t sizeofAvail = (3 + queue.queueNum) * 2;
    
    // struct virtq_avail {
    //     #define VIRTQ_AVAIL_F_NO_INTERRUPT 1
    //     le16 flags;
    //     le16 idx;
    //     le16 ring[ /* Queue Size */ ];
    //     le16 used_event; /* Only if VIRTIO_F_EVENT_IDX */
    // };
    
    uint16_t *avail = (uint16_t *)this->bus->get_ptr(queue.p_avail, sizeofAvail);
    if (avail == nullptr) {
        WARN("Failed to get struct virtq_avail");
        return ;
    }
    uint16_t availIdx = avail[1];
    uint16_t *ring = avail + 2;
    uint16_t descIdx = ring[availIdx];
    
    const VirtQueueDescriptor *descriptors = (VirtQueueDescriptor *)this->bus->get_ptr(queue.p_desc, sizeof(VirtQueueDescriptor) * queue.queueNum);
    if (descriptors == nullptr) {
        WARN("Failed to get struct virtq_desc");
        return ;
    }
    
    VirtQueueDescriptor descriptor = descriptors[descIdx];
    
    if (descriptor.flags & VIRTQ_DESC_F_INDIRECT) {
        NOT_IMPLEMENTED();
    }
    
    std::vector<Buffer> buffer;
    while (descriptor.flags & VIRTQ_DESC_F_NEXT) {
        buffer.push_back({
            descriptor.addr, 
            descriptor.len, 
            (descriptor.flags & VIRTQ_DESC_F_WRITE) != 0,
        });
        descriptor = descriptors[descriptor.next];
    }

    uint32_t len;
    if (this->virtio_handle_req(buffer, len)) {
        virtio_handle_done(len, queueIdx, descIdx);
    } else {
        WARN("Occured an error");
    }
}

void VirtIO::virtio_handle_done(uint32_t len, unsigned int queueIndex, unsigned int descIndex) {
    const VirtQueue &queue = this->virtQueues[queueIndex];
    
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
    
    void *used = this->bus->get_ptr(6 + sizeof(VirtQueueUsedElem) * queue.queueNum);
    bool noNotify = ((uint16_t *)used)[0] & VIRTQ_USED_F_NO_NOTIFY;
    
    uint16_t usedIndex = ((uint16_t *)used)[1];
    VirtQueueUsedElem *ring = (VirtQueueUsedElem *) (((char *)used) + 4);
    ring[usedIndex].id  = descIndex;
    ring[usedIndex].len = len;
    
    ((uint16_t *)used)[1] ++; // Increase the used->idx

    if (noNotify) {
        this->interrupt = true;
    }
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

        case 0x60: return 0; // InterruptStatus field, not support Configuration Change Interrupt yet
        case 0x70: return this->read_status();
    }

    valid = false;
    return -1;
}

bool VirtIO::write(word_t offset, word_t data, word_t size) {
    // Configuration Map
    if (offset >= 0x100 && offset + size <= 0x100 + this->sizeof_configuration) {
        offset -= 0x100;
        char *c = new char[this->sizeof_configuration];
        void *p = c + offset;
        
        switch (size) {
            case 1: *(uint8_t  *)p = data;
            case 2: *(uint16_t *)p = data;
            case 4: *(uint32_t *)p = data;
            case 8: *(uint64_t *)p = data;
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
        case 0x38: this->virtQueues[this->queueSelect].queueNum = data;
        case 0x44: this->virtQueues[this->queueSelect].ready = data == 1;
        
        case 0x64: break; // Interrupt ACK, not implenmted ignore yet
        case 0x70: this->write_status(data); break;

        case 0x80: SET_UPPER(this->virtQueues[this->queueSelect].p_desc , data); break;
        case 0x84: SET_LOWER(this->virtQueues[this->queueSelect].p_desc , data); break;
        case 0x90: SET_UPPER(this->virtQueues[this->queueSelect].p_used , data); break;
        case 0x94: SET_LOWER(this->virtQueues[this->queueSelect].p_used , data); break;
        case 0xa0: SET_UPPER(this->virtQueues[this->queueSelect].p_avail, data); break;
        case 0xa4: SET_LOWER(this->virtQueues[this->queueSelect].p_avail, data); break;
        
        default: valid = false; break;
    }

    return valid;
}

bool VirtIO::interrupt_pending() {
    return this->interrupt;
}

void VirtIO::clear_interrupt() {
    this->interrupt = false;
}
