#include "device/uart.h"
#include "isa/word.h"
#include "log.h"

#include <cstdint>
#include <iostream>

#define LSR_RX_READY (1 << 0)
#define LSR_TX_READY (1 << 5)

#define BUFFER_SIZE 1024

word_t Uart16650::read(word_t offset, int size) {
    if (size != 1) {
        WARN("uart read size %d not supported.", size);
        return 0;
    }
    if (offset == 0) {
        if (lcr & 0x80) { // Divisor Latch Access Bit is set
            // LSB: Lower 8 bits of the divisor latch
            return lsb;
        } else {
            // RBR: Receiver Buffer Register
            if (queue.empty()) {
                return 0;
            }
            uint8_t c = queue.front();
            queue.pop();
            if (queue.empty()) {
                lsr &= ~LSR_RX_READY;
            }
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
        WARN("uart read offset %d not supported.", offset);
        return -1;
    }
}

bool Uart16650::write(word_t offset, word_t data, int size) {
    if (size != 1) {
        WARN("uart write size %d not supported.", size);
        return false;
    }
    if (offset == 0) {
        if (lcr & 0x80) { // Divisor Latch Access Bit is set
            // LSB: Lower 8 bits of the divisor latch
            lsb = data;
            return true;
        } else {
            // THR: Transmitter Holding Register
            send_byte(data);
            return true;
        }
    } else if (offset == 1) {
        if (lcr & 0x80) { // Divisor Latch Access Bit is set
            // MSB: Upper 8 bits of the divisor latch
            msb = data;
            return true;
        } else {
            // IER: Interrupt Enable Register
            ier = data;
            return true;
        }
    } else if (offset == 3) {
        // LCR: Line Control Register
        lcr = data;
        return true;
    } else if (offset == 4) {
        // Note: MCR is not implemented
        return true;
    } else {
        WARN("uart write offset %d not supported.", offset);
        return false;
    }
}

bool Uart16650::putch(uint8_t data) {
    if (queue.size() >= BUFFER_SIZE) {
        return false;
    }
    queue.push(data);
    lsr |= LSR_RX_READY;
    return true;
}

void Uart16650::set_output_stream(std::ostream &os) {
    if (mode == Mode::SOCKET) {
        // TODO: close socket and free socket resources
    } 
    mode = Mode::STREAM;
    stream = &os;
    // for now, we assume that the stream is always ready to write
    lsr |= LSR_TX_READY;
}

void Uart16650::open_socket(const std::string &ip, int port) {
    if (mode == Mode::STREAM) {
        stream = nullptr;
    }
    mode = Mode::SOCKET;
    // TODO: open socket
    lsr &= ~LSR_TX_READY;
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
    } else if (mode == Mode::SOCKET) {
        // TODO: send data to socket
    }
}

uint8_t *Uart16650::get_ptr(word_t offset) {
    return nullptr;
}

std::string Uart16650::get_type_str() const {
    return "uart16650";
}
