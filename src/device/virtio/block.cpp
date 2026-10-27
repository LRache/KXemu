#include "device/virtio/block.hpp"
#include "device/def.hpp"
#include "device/virtio/virtio.hpp"
#include "device/virtio/def.h"
#include "log.h"

#include <asio/post.hpp>
#include <asio/thread_pool.hpp>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace kxemu::device;

static constexpr uint64_t SECTOR_SIZE = 512;

static constexpr uint8_t VIRTIO_BLK_S_OK     = 0;
static constexpr uint8_t VIRTIO_BLK_S_IOERR  = 1;
static constexpr uint8_t VIRTIO_BLK_S_UNSUPP = 2;

static constexpr char BLK_ID[] = "kxemu-virtio-blk";
static constexpr std::size_t BLK_ID_BYTES = 20;

// struct virtio_blk_req header
struct BufferHead {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
};

VirtIOBlock::VirtIOBlock(unsigned int threads)
    : VirtIO(VIRTIO_BLK_DEVICE_ID, 15, 1),
      pool(std::make_unique<asio::thread_pool>(std::max(1u, threads))) {
    this->config.capacity = 0;
    this->configuration = &this->config;
    this->sizeof_configuration = sizeof(this->config);
}

VirtIOBlock::~VirtIOBlock() {
    // Every queued request completes before the base class and the fd go away.
    this->pool->join();
    if (this->fd >= 0) {
        close(this->fd);
    }
}

bool VirtIOBlock::open_raw_img(const std::string &filepath) {
    int fd = open(filepath.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return false;
    }
    if (this->fd >= 0) {
        close(this->fd);
    }
    this->fd = fd;
    this->imgSize = st.st_size;
    this->config.capacity = this->imgSize / SECTOR_SIZE;
    return true;
}

bool VirtIOBlock::get_device_features_bit(unsigned int bit) {
    return bit == VIRTIO_BLK_F_FLUSH;
}

// Read or write the whole iovec at offset, retrying short transfers.
static bool transfer_all(int fd, bool write, std::vector<iovec> iov, uint64_t offset) {
    std::size_t first = 0;
    while (first < iov.size()) {
        const int count = std::min<std::size_t>(iov.size() - first, IOV_MAX);
        ssize_t n = write ? pwritev(fd, &iov[first], count, offset)
                          : preadv (fd, &iov[first], count, offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        offset += n;
        while (n > 0) {
            const std::size_t step = std::min<std::size_t>(n, iov[first].iov_len);
            iov[first].iov_base = (char *)iov[first].iov_base + step;
            iov[first].iov_len -= step;
            n -= step;
            if (iov[first].iov_len == 0) first++;
        }
        while (first < iov.size() && iov[first].iov_len == 0) first++;
    }
    return true;
}

uint8_t VirtIOBlock::serve(uint32_t type, uint64_t sector, std::vector<Buffer> &data, uint32_t &written) {
    written = 0;
    uint64_t length = 0;
    std::vector<iovec> iov;
    for (const auto &buffer : data) {
        length += buffer.len;
        if (buffer.len != 0) iov.push_back({buffer.ptr, buffer.len});
    }

    switch (type) {
        case VIRTIO_BLK_T_IN:
        case VIRTIO_BLK_T_OUT: {
            const bool write = type == VIRTIO_BLK_T_OUT;
            if (length % SECTOR_SIZE != 0 || sector > this->config.capacity
                || length / SECTOR_SIZE > this->config.capacity - sector) {
                WARN("virtio-blk: request out of range, sector=%lu, length=%lu",
                     (unsigned long)sector, (unsigned long)length);
                return VIRTIO_BLK_S_IOERR;
            }
            for (const auto &buffer : data) {
                // Reads fill device-writable buffers, writes take driver-written ones.
                if (buffer.write != !write) return VIRTIO_BLK_S_IOERR;
            }
            if (!transfer_all(this->fd, write, std::move(iov), sector * SECTOR_SIZE)) {
                WARN("virtio-blk: %s failed: %s", write ? "write" : "read", strerror(errno));
                return VIRTIO_BLK_S_IOERR;
            }
            if (!write) written = length;
            return VIRTIO_BLK_S_OK;
        }
        case VIRTIO_BLK_T_FLUSH:
            return fdatasync(this->fd) == 0 ? VIRTIO_BLK_S_OK : VIRTIO_BLK_S_IOERR;
        case VIRTIO_BLK_T_GET_ID: {
            // A NUL-padded serial of at most 20 bytes.
            uint8_t id[BLK_ID_BYTES] = {};
            std::memcpy(id, BLK_ID, std::min(sizeof(BLK_ID) - 1, BLK_ID_BYTES));
            for (const auto &buffer : data) {
                const std::size_t n = std::min<std::size_t>(buffer.len, BLK_ID_BYTES - written);
                if (!buffer.write) return VIRTIO_BLK_S_IOERR;
                std::memcpy(buffer.ptr, id + written, n);
                written += n;
            }
            return VIRTIO_BLK_S_OK;
        }
        default:
            WARN("virtio-blk: unsupported request type %u", type);
            return VIRTIO_BLK_S_UNSUPP;
    }
}

void VirtIOBlock::virtio_submit(Request &&req) {
    // Layout: a 16-byte header, the data, then a 1-byte status at the very end.
    // Descriptor boundaries are not significant, so split by bytes.
    std::vector<Buffer> data;
    BufferHead head;
    std::size_t headCopied = 0;
    uint8_t *status = nullptr;
    for (std::size_t i = 0; i < req.buffers.size(); i++) {
        Buffer buffer = req.buffers[i];
        if (headCopied < sizeof(head)) {
            const std::size_t n = std::min<std::size_t>(buffer.len, sizeof(head) - headCopied);
            std::memcpy((uint8_t *)&head + headCopied, buffer.ptr, n);
            headCopied += n;
            buffer.ptr += n;
            buffer.len -= n;
        }
        if (i + 1 == req.buffers.size() && buffer.len != 0 && buffer.write) {
            status = buffer.ptr + buffer.len - 1;
            buffer.len -= 1;
        }
        if (buffer.len != 0) data.push_back(buffer);
    }

    if (headCopied < sizeof(head) || status == nullptr || this->fd < 0) {
        WARN("virtio-blk: malformed request, head=%u", (unsigned)req.head);
        if (status != nullptr) *status = VIRTIO_BLK_S_IOERR;
        this->virtio_complete_locked(req, status != nullptr ? 1 : 0);
        if (--this->inflight == 0) this->idle.notify_all();
        return;
    }

    asio::post(*this->pool, [this, req = std::move(req), data = std::move(data), head, status]() mutable {
        uint32_t written;
        *status = this->serve(head.type, head.sector, data, written);
        this->virtio_complete(req, written + 1);
    });
}
