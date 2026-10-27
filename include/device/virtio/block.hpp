#ifndef __KXEMU_DEVICE_VIRTIO_BLOCK_HPP__
#define __KXEMU_DEVICE_VIRTIO_BLOCK_HPP__

#include "device/def.hpp"
#include "device/virtio/virtio.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace asio {
class thread_pool;
}

namespace kxemu::device {

// Requests are parsed on the hart that writes QueueNotify and served on a
// thread pool with preadv/pwritev straight into guest memory. They may complete
// out of order; each completion updates the used ring and raises the IRQ.
class VirtIOBlock : public VirtIO {
private:
    void virtio_submit(Request &&req) override;
    bool get_device_features_bit(unsigned int bit) override;

    // Runs on the pool. Returns the virtio-blk status and the bytes written to data.
    uint8_t serve(uint32_t type, uint64_t sector, std::vector<Buffer> &data, uint32_t &written);

    int fd = -1;
    uint64_t imgSize = 0;
    std::unique_ptr<asio::thread_pool> pool;

    struct Config {
        uint64_t capacity; // In 512-byte sectors
    } config;

public:
    explicit VirtIOBlock(unsigned int threads = 4);
    ~VirtIOBlock();

    bool open_raw_img(const std::string &filepath);
};

} // namepace kxemu::device

#endif
