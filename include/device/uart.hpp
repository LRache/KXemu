#ifndef __KXEMU_DEVICE_UART_HPP__
#define __KXEMU_DEVICE_UART_HPP__

#include "device/mmio.hpp"
#include "utils/io-loop.hpp"

#include <cstdint>
#include <memory>
#include <ostream>
#include <queue>
#include <mutex>

#define UART_LENGTH 8

namespace kxemu::device {

class Uart16650: public MMIODev {
public:
    word_t read(word_t offset, word_t size, bool &valid) override;
    bool  write(word_t offset, word_t data, word_t size) override;
    
    const char *get_type_name() const override;

    bool putch(uint8_t data);
    void set_output_stream(std::ostream &os);    // send data to stream
    // Connect to ip:port, which carries both input and output, and run I/O on loop.
    // Stop the loop before destroying the UART, and destroy the UART before the loop.
    bool open_socket(utils::IOLoop &loop, const std::string &ip, int port);
    // Receive from readFd and send to writeFd on loop: pipes, ttys, sockets and the
    // like. Pass the same fd twice for a full-duplex one such as a socket. The UART
    // owns and closes them, also when this fails.
    bool set_fds(utils::IOLoop &loop, int readFd, int writeFd);

    enum Mode {
        NONE,
        STREAM,
        FD,
    };
    
    Uart16650();
    ~Uart16650();

private:
    int mode = Mode::NONE;
    std::ostream *stream = nullptr;
    
    // Asio descriptors; both receive and send run on the I/O loop.
    struct FdIO;
    std::unique_ptr<FdIO> fds;
    void start_receive();

    // Guards the registers, the FIFO and the IRQ level. The IRQ is driven with it
    // held so that level changes reach the interrupt controller in order.
    std::mutex mtx;
    std::queue<uint8_t> queue; // FIFO buffer
    bool recv_byte_locked(uint8_t c);

    void send_byte(uint8_t c);

    bool threPending = false; // THR empty interrupt, cleared by IIR read or THR write
    bool irqLevel = false;
    uint8_t iir_locked() const;
    void update_irq_locked();

    // Divisor Latch Register
    // [0-7] RW - Divisor Latch
    uint8_t lsb = 0x00; // Divisor Latch Low
    uint8_t msb = 0x00; // Divisor Latch High

    // Interrupt Enable Register
    // [0] RW - Enable Received Data Available Interrupt
    // [1] RW - Enable Transmitter Holding Register Empty Interrupt
    // [2] RW - Enable Receiver Line Status Interrupt
    // [3] RW - Enable Modem Status Interrupt
    // [4-7] RW - Reserved
    uint8_t ier = 0b00000000; // reset value

    // Interrupt Identification Register, derived from the state by iir_locked()
    // [0-3] R - Interrupt Identification
    // [4-5] R - 0
    // [6-7] R - 1

    // FIFO Control Register
    // [0] W - Ignored
    // [1] W - Clear Receive FIFO
    // [2] W - Clear Transmit FIFO
    // [3-5] W - Ignored
    // [6-7] R - Receiver FIFO Trigger Level
    // uint8_t fcr = 0b11000000; // reset value

    // Line Control Register
    // [0-1] RW - Word Length
    // [2] RW - Stop Bit
    // [3] RW - Parity Enable
    // [4] RW - Parity Type
    // [5] RW - Strick Parity
    // [6] RW - Break Control
    // [7] RW - Divisor Latch Access Bit
    uint8_t lcr = 0b00000011; // reset value

    // Line Status Register
    // [0] R - Data Ready
    // [1] R - Overrun Error
    // [2] R - Parity Error
    // [3] R - Framing Error
    // [4] R - Break Interrupt
    // [5] R - Transmitter Holding Register Empty
    // [6] R - Transmitter Empty
    // [7] R - FIFO Data Error
    uint8_t lsr = 0b00100000; // reset value

    // NOTE: The following registers are not implemented
    // Modem Status Register
    // [0] R - Delta Clear to Send
    // [1] R - Delta Data Set Ready
    // [2] R - Trailing Edge Ring Indicator
    // [3] R - Delta Data Carrier Detect
    uint8_t msr = 0;

    unsigned int recvFIFOTriggerByteCount = 1;
};

} // namespace kxemu::device

#endif
