#include "device/uart.hpp"
#include "log.h"
#include "word.h"

#include <asio/as_tuple.hpp>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <asio/post.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>

#include <cstdint>
#include <iostream>
#include <mutex>
#include <string>
#include <unistd.h>

#define LSR_RX_READY (1 << 0)
#define LSR_TX_READY (1 << 5)

#define BUFFER_SIZE 1024

using namespace kxemu::device;

struct Uart16650::FdIO {
    // Input and output descriptors; each has its own fd, also for a full-duplex one.
    asio::posix::stream_descriptor in;
    asio::posix::stream_descriptor out;

    explicit FdIO(asio::io_context &context) : in(context), out(context) {}

    asio::awaitable<void> recv_loop(Uart16650 &uart);
    // Thread-safe; the write itself runs asynchronously on the loop.
    void send_byte(uint8_t c);

private:
    static constexpr std::size_t SEND_QUEUE_MAX = 1 << 20;

    // Send state, touched only on the loop thread.
    std::string sendQueue;  // Bytes waiting for the next write
    std::string sendBuffer; // Bytes of the write in progress
    bool sending = false;
    bool sendFailed = false;
    void start_send();
};

// IIR interrupt identification, highest priority first.
static constexpr uint8_t IIR_NONE    = 0b0001;
static constexpr uint8_t IIR_RDA     = 0b0100; // Received data available
static constexpr uint8_t IIR_TIMEOUT = 0b1100; // Character timeout
static constexpr uint8_t IIR_THRE    = 0b0010; // Transmitter holding register empty
static constexpr uint8_t IIR_FIFO    = 0b11000000;

static constexpr uint8_t IER_RDA  = 1 << 0;
static constexpr uint8_t IER_THRE = 1 << 1;

uint8_t Uart16650::iir_locked() const {
    if ((ier & IER_RDA) && !queue.empty()) {
        // Data below the trigger level is reported at once as a character timeout.
        return IIR_FIFO | (queue.size() >= recvFIFOTriggerByteCount ? IIR_RDA : IIR_TIMEOUT);
    }
    if ((ier & IER_THRE) && threPending) {
        return IIR_FIFO | IIR_THRE;
    }
    return IIR_FIFO | IIR_NONE;
}

void Uart16650::update_irq_locked() {
    const bool level = (iir_locked() & IIR_NONE) == 0;
    if (level != irqLevel) {
        irqLevel = level;
        this->set_irq_level(level);
    }
}

word_t Uart16650::read(word_t offset, word_t size, bool &valid) {
    if (size != 1) {
        WARN("uart read size " FMT_VARU64 " not supported.", size);
        valid = false;
        return -1;
    }

    valid = true;
    std::lock_guard<std::mutex> lock(mtx);
    if (offset == 0) {
        if (lcr & 0x80) { // Divisor Latch Access Bit is set
            // LSB: Lower 8 bits of the divisor latch
            return lsb;
        } else {
            // RBR: Receiver Buffer Register
            if (queue.empty()) {
                return -1;
            }
            uint8_t c = queue.front();
            queue.pop();
            if (queue.empty()) {
                lsr &= ~LSR_RX_READY;
            }
            // Draining the FIFO clears the received data interrupt.
            update_irq_locked();
            return c;
        }
    } else if (offset == 1) {
        if (lcr & 0x80) { // Divisor Latch Access Bit is set
            // MSB: Upper 8 bits of the divisor latch
            return msb;
        } else {
            // IER: Interrupt Enable Register
            return ier;
        }
    } else if (offset == 2) {
        // IIR: Interrupt Identification Register
        uint8_t iir = iir_locked();
        if ((iir & 0x0f) == IIR_THRE) {
            // Reading IIR acknowledges a THR empty interrupt.
            threPending = false;
            update_irq_locked();
        }
        return iir;
    } else if (offset == 3) {
        // LCR: Line Control Register
        return lcr;
    } else if (offset == 5) {
        // LSR: Line Status Register
        return lsr;
    } else if (offset == 6) {
        // MSR: Modem Status Register
        return msr;
    } else {
        WARN("uart read offset " FMT_VARU64 " not supported.", offset);
        valid = false;
        return -1;
    }
}

bool Uart16650::write(word_t offset, word_t data, word_t size) {
    if (size != 1) {
        WARN("uart write size " FMT_VARU64 " not supported.", size);
        return false;
    }
    std::lock_guard<std::mutex> lock(mtx);
    if (offset == 0) {
        if (lcr & 0x80) { // Divisor Latch Access Bit is set
            // LSB: Lower 8 bits of the divisor latch
            lsb = data;
            return true;
        } else {
            // THR: Transmitter Holding Register
            send_byte(data);
            // Transmission completes at once, so THR is empty again.
            threPending = true;
            update_irq_locked();
            return true;
        }
    } else if (offset == 1) {
        if (lcr & 0x80) { // Divisor Latch Access Bit is set
            // MSB: Upper 8 bits of the divisor latch
            msb = data;
            return true;
        } else {
            // IER: Interrupt Enable Register
            // Enabling the THR empty interrupt while THR is empty raises it.
            if ((data & IER_THRE) && !(ier & IER_THRE) && (lsr & LSR_TX_READY)) {
                threPending = true;
            }
            ier = data;
            update_irq_locked();
            return true;
        }
    } else if (offset == 2) {
        // FCR: FIFO Control Register
        if (data & (1 << 1)) {
            // Clear Receive FIFO
            while (!queue.empty()) {
                queue.pop();
            }
            lsr &= ~LSR_RX_READY;
        }

        switch ((data & 0b11000000) >> 6) {
            case 0b00: recvFIFOTriggerByteCount = 1; break;
            case 0b01: recvFIFOTriggerByteCount = 4; break;
            case 0b10: recvFIFOTriggerByteCount = 8; break;
            case 0b11: recvFIFOTriggerByteCount = 14; break;
        }

        update_irq_locked();
        return true;
    } else if (offset == 3) {
        // LCR: Line Control Register
        lcr = data;
        return true;
    } else if (offset == 4) {
        // Note: MCR is not implemented
        return true;
    } else {
        WARN("uart write offset " FMT_VARU64 " not supported.", offset);
        return true;
    }
}

asio::awaitable<void> Uart16650::FdIO::recv_loop(Uart16650 &uart) {
    uint8_t buffer[64];
    while (true) {
        auto [ec, n] = co_await this->in.async_read_some(
            asio::buffer(buffer), asio::as_tuple(asio::use_awaitable));
        if (ec) {
            // operation_aborted: the fd was closed with the UART.
            if (ec != asio::error::operation_aborted) {
                WARN("uart input closed, stop receiving: %s", ec.message().c_str());
            }
            co_return;
        }

        std::lock_guard<std::mutex> lock(uart.mtx);
        for (std::size_t i = 0; i < n; i++) {
            uart.recv_byte_locked(buffer[i]);
        }
        uart.update_irq_locked();
    }
}

void Uart16650::start_receive() {
    asio::co_spawn(fds->in.get_executor(), fds->recv_loop(*this), asio::detached);
}

bool Uart16650::putch(uint8_t data) {
    std::lock_guard<std::mutex> lock(mtx);
    if (!recv_byte_locked(data)) {
        return false;
    }
    update_irq_locked();
    return true;
}

void Uart16650::set_output_stream(std::ostream &os) {
    if (mode == Mode::FD) {
        PANIC("Cannot change to use stream.");
    } 
    mode = Mode::STREAM;
    stream = &os;
    // For now, we assume that the stream is always ready to write
    lsr |= LSR_TX_READY;
}

bool Uart16650::open_socket(utils::IOLoop &loop, const std::string &ip, int port) {
    if (mode != Mode::NONE) {
        PANIC("Cannot change to use socket.");
        return false;
    }

    asio::error_code ec;
    const auto address = asio::ip::make_address(ip, ec);
    if (ec) {
        WARN("Invalid address %s: %s", ip.c_str(), ec.message().c_str());
        return false;
    }
    asio::ip::tcp::socket socket(loop.context());
    socket.connect({address, static_cast<asio::ip::port_type>(port)}, ec);
    if (ec) {
        WARN("Failed to connect %s:%d: %s", ip.c_str(), port, ec.message().c_str());
        return false;
    }

    const int fd = socket.release(ec);
    if (ec) {
        WARN("Failed to release socket %s:%d: %s", ip.c_str(), port, ec.message().c_str());
        return false;
    }
    // A TCP socket is full duplex, so the same fd carries input and output.
    return set_fds(loop, fd, fd);
}

bool Uart16650::set_fds(utils::IOLoop &loop, int readFd, int writeFd) {
    if (mode != Mode::NONE) {
        PANIC("Cannot change to use fds.");
    }

    auto io = std::make_unique<FdIO>(loop.context());

    // A full-duplex fd serves both directions. Give output its own duplicate, so that
    // each descriptor registers with the loop and closes on its own.
    const int outFd = (writeFd == readFd && readFd >= 0) ? dup(readFd) : writeFd;

    // Ownership passes even on failure: an adopted fd closes with io, any other here.
    auto adopt = [](asio::posix::stream_descriptor &desc, int fd) {
        if (fd < 0) return false;
        asio::error_code ec;
        desc.assign(fd, ec);
        if (ec) {
            close(fd);
            return false;
        }
        return true;
    };
    // Adopt both before checking, so that neither fd is left unowned.
    const bool inAdopted = adopt(io->in, readFd);
    const bool outAdopted = adopt(io->out, outFd);
    if (!inAdopted || !outAdopted) {
        WARN("Failed to adopt uart fds.");
        return false;
    }

    fds = std::move(io);
    mode = Mode::FD;
    lsr |= LSR_TX_READY;
    start_receive();
    return true;
}

bool Uart16650::recv_byte_locked(uint8_t c) {
    if (queue.size() >= BUFFER_SIZE) {
        WARN("uart buffer is full, drop data.");
        return false;
    }
    queue.push(c);
    lsr |= LSR_RX_READY;
    return true;
}

void Uart16650::send_byte(uint8_t c) {
    if (mode == Mode::STREAM) {
        if (stream == nullptr) {
            WARN("uart stream is not set, output to stdout.");
            std::cout << c;
            std::cout.flush();
        } else {
            *stream << c;
            stream->flush();
        } 
    } else if (mode == Mode::FD) {
        fds->send_byte(c);
    }
}

void Uart16650::FdIO::send_byte(uint8_t c) {
    // Hand the byte to the loop: the hart holds the UART mutex here, so it must not
    // block on a peer that stops reading. Posts run in order, so bytes keep theirs.
    asio::post(this->out.get_executor(), [this, c] {
        if (this->sendFailed) return;
        if (this->sendQueue.size() >= SEND_QUEUE_MAX) {
            WARN("uart send queue is full, drop data.");
            return;
        }
        this->sendQueue.push_back(char(c));
        if (!this->sending) this->start_send();
    });
}

void Uart16650::FdIO::start_send() {
    this->sending = true;
    this->sendBuffer = std::move(this->sendQueue);
    this->sendQueue.clear();
    asio::async_write(this->out, asio::buffer(this->sendBuffer),
                      [this](const asio::error_code &ec, std::size_t) {
        this->sending = false;
        if (ec) {
            // operation_aborted: the fd was closed with the UART.
            if (ec != asio::error::operation_aborted) {
                WARN("uart output closed, stop sending: %s", ec.message().c_str());
            }
            this->sendFailed = true;
            this->sendQueue.clear();
            return;
        }
        if (!this->sendQueue.empty()) this->start_send();
    });
}

const char *Uart16650::get_type_name() const {
    return "uart16650";
}

Uart16650::Uart16650() = default;
Uart16650::~Uart16650() = default;
