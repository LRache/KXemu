#include "device/virtio/block.h"
#include "device/def.h"
#include "device/virtio/virtio.h"
#include "device/virtio/def.h"
#include <cstdint>
#include <cstring>
#include <iterator>

using namespace kxemu::device;

VirtIOBlock::VirtIOBlock() : VirtIO(VIRTIO_BLK_DEVICE_ID, 15, 1) {
    
}

bool VirtIOBlock::open_raw_img(const std::string &filepath) {
    this->fstream.open(filepath, std::ios::in | std::ios::out | std::ios::binary);
    if (!this->fstream.is_open()) {
        return false;
    }
    this->fstream.seekg(0, std::ios::end);
    this->imgSize = this->fstream.tellg();
    this->fstream.seekg(0, std::ios::beg);
    return true;
}

bool VirtIOBlock::blk_read (uint32_t sector, uint32_t len, word_t bufferAddr, uint8_t *status) {
    if (len & ~(512 - 1)) return false;
    char *dest = (char *)this->bus->get_ptr(bufferAddr, len);
    if (dest == nullptr) {
        return false;
    }
    
    std::size_t start = sector * 512;
    if (start + len >= this->imgSize) {
        return false;
    }
    
    char *buffer = new char[len];
    this->fstream.seekg(start, std::ios::beg);
    this->fstream.read(buffer, len);
    if (this->fstream.fail()) {
        return false;
    }
    std::memcpy(dest, buffer, len);

    return true;
}
bool VirtIOBlock::blk_write(uint32_t sector, uint32_t len, word_t bufferAddr, uint8_t *status) {
    if (len & ~(512 - 1)) return false;

    char *buffer = (char *)this->bus->get_ptr(bufferAddr, len);
    if (buffer == nullptr) {
        return false;
    }

    this->fstream.write(buffer, len);
    if (this->fstream.fail()) {
        return false;
    }

    return true;
}

bool VirtIOBlock::virtio_handle_req(const std::vector<Buffer> &buffer, uint32_t &len) {
    if (buffer.size() < 3) {
        return false;
    }
    
    const Buffer &b0 = buffer[0];
    const Buffer &b1 = buffer[1];
    const Buffer &b2 = buffer[2];
    
    const BufferHead *head = (BufferHead *)this->bus->get_ptr(b0.addr, sizeof(BufferHead));
    if (head == nullptr) {
        return false;
    }

    uint8_t *status = (uint8_t *)this->bus->get_ptr(b2.addr, sizeof(uint8_t));
    
    switch (head->type) {
        case VIRTIO_BLK_T_IN:  return this->blk_read (head->sector, b1.len, b1.addr, status);
        case VIRTIO_BLK_T_OUT: return this->blk_write(head->sector, b1.len, b1.addr, status);
        default: return false;
    }
}

VirtIOBlock::~VirtIOBlock() {
    if (this->fstream.is_open()) {
        this->fstream.close();
    }
}
