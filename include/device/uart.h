#ifndef __KXEMU_DEVICE_UART_H__
#define __KXEMU_DEVICE_UART_H__

#include "device/mmio.h"

#include <atomic>
#include <cstdint>
#include <ostream>
#include <queue>
#include <thread>
#include <mutex>

#define UART_LENGTH 8

namespace kxemu::device {

class Uart16650: public MMIOMap {
public:
    word_t read(word_t offset, word_t size, bool &valid) override;
    bool  write(word_t offset, word_t data, word_t size) override;
    void update() override;
    bool has_interrupt() override;
    
    const char *get_type_name() const override;

    bool putch(uint8_t data);
    void set_output_stream(std::ostream &os);    // send data to stream
    bool open_socket(const std::string &ip, int port); // open socket to send and receive data

    enum Mode {
        NONE,
        STREAM,
        SOCKET,
    };
    
    ~Uart16650();

private:
    int mode = Mode::NONE;
    std::ostream *stream = nullptr;
    
    int sendSocket = -1;
    int recvSocket = -1;

    std::atomic<bool> uartSocketRunning;
    std::thread *recvThread;
    void recv_thread_loop();

    std::mutex queueMtx;
    std::queue<uint8_t> queue; // FIFO buffer
    
    std::mutex senderMtx;
    void send_byte(uint8_t c);

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

    // Interrupt Identification Register
    // [0-3] R - Interrupt Identification
    // [4-5] R - 0
    // [6-7] R - 1
    uint8_t iir = 0b11000001; // reset value

    // FIFO Control Register
    // [0] W - Ignored
    // [1] W - Clear Receive FIFO
    // [2] W - Clear Transmit FIFO
    // [3-5] W - Ignored
    // [6-7] R - Receiver FIFO Trigger Level
    uint8_t fcr = 0b11000000; // reset value

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
    uint8_t lsr;

    // NOTE: The following registers are not implemented
    // Modem Status Register
    // [0] R - Delta Clear to Send
    // [1] R - Delta Data Set Ready
    // [2] R - Trailing Edge Ring Indicator
    // [3] R - Delta Data Carrier Detect
    uint8_t msr;
};

} // namespace kxemu::device

#endif
