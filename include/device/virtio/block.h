#ifndef __KXEMU_DEVICE_VIRTIO_BLOCK_H__
#define __KXEMU_DEVICE_VIRTIO_BLOCK_H__

#include "device/def.h"
#include "device/virtio/virtio.h"
#include <cstdint>
#include <fstream>

namespace kxemu::device {

class VirtIOBlock : public VirtIO {
private:
    bool virtio_handle_req(const std::vector<Buffer> &buffer, uint32_t &len) override;

    bool blk_read (uint32_t sector, uint32_t len, word_t bufferAddr, uint8_t *status);
    bool blk_write(uint32_t sector, uint32_t len, word_t bufferAddr, uint8_t *status);

    std::fstream fstream;
    std::size_t imgSize;

    struct BufferHead {
        uint32_t type;
        uint32_t reserved;
        uint64_t sector;
    };

public:
    VirtIOBlock();
    ~VirtIOBlock();

    bool open_raw_img(const std::string &filepath);
};

} // namepace kxemu::device

#endif
