#include "device/virtio/block.h"
#include "device/def.h"
#include "device/virtio/virtio.h"
#include "device/virtio/def.h"
#include "log.h"

#include <cstring>
#include <mutex>

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

bool VirtIOBlock::blk_read(uint32_t sector, uint32_t len, word_t bufferAddr, uint8_t *status) {
    if (len & (512 - 1)) return false;
    
    char *dest = (char *)this->bus->get_ptr(bufferAddr, len);
    if (dest == nullptr) {
        WARN("dest is nullptr");
        return false;
    }
    
    std::size_t start = sector * 512;
    if (start + len >= this->imgSize) {
        WARN("start + len >= this->imgSize");
        *status = 0;
        return true;
    }

    std::lock_guard<std::mutex> lock(this->streamMtx);
    
    char *buffer = new char[len];
    this->fstream.seekg(start, std::ios::beg);
    this->fstream.read(buffer, len);
    if (this->fstream.fail()) {
        WARN("this->fstream.fail()");
        delete []buffer;
        return false;
    }
    std::memcpy(dest, buffer, len);
    delete []buffer;

    *status = 0;

    return true;
}
bool VirtIOBlock::blk_write(uint32_t sector, uint32_t len, word_t bufferAddr, uint8_t *status) {
    if (len & (512 - 1)) return false;

    char *buffer = (char *)this->bus->get_ptr(bufferAddr, len);
    if (buffer == nullptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(this->streamMtx);

    std::size_t start = sector * 512;
    if (start + len >= this->imgSize) {
        WARN("start + len >= this->imgSize");
        *status = 0;
        return true;
    }

    this->fstream.seekp(start, std::ios::beg);

    this->fstream.write(buffer, len);
    if (this->fstream.fail()) {
        return false;
    }
    this->fstream.flush();

    *status = 0;

    return true;
}

bool VirtIOBlock::virtio_handle_req(const std::vector<Buffer> &buffer, uint32_t &len) {
    if (buffer.size() < 3) {
        const BufferHead *head = (BufferHead *)this->bus->get_ptr(buffer[0].addr, sizeof(BufferHead));
        if (head != nullptr) {
            INFO("head->sector: %lu", head->sector);
        } else {
            WARN("head is nullptr, head.addr: " FMT_WORD64, buffer[0].addr);
        }
        WARN("buffer.size() < 3: %lu", buffer.size());
        return true;
    }
    
    const Buffer &b0 = buffer[0];
    const Buffer &b1 = buffer[1];
    const Buffer &b2 = buffer[2];
    
    const BufferHead *head = (BufferHead *)this->bus->get_ptr(b0.addr, sizeof(BufferHead));
    if (head == nullptr) {
        WARN("head is nullptr");
        return false;
    }

    uint8_t *status = (uint8_t *)this->bus->get_ptr(b2.addr, sizeof(uint8_t));
    
    switch (head->type) {
        case VIRTIO_BLK_T_IN:  return this->blk_read (head->sector, b1.len, b1.addr, status);
        case VIRTIO_BLK_T_OUT: return this->blk_write(head->sector, b1.len, b1.addr, status);
        default: WARN("Unknown type: %u", head->type); return false;
    }
}

VirtIOBlock::~VirtIOBlock() {
    if (this->fstream.is_open()) {
        this->fstream.close();
    }
}
