#include "device/uart.h"
#include "log.h"
#include "word.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define LSR_RX_READY (1 << 0)
#define LSR_TX_READY (1 << 5)

#define BUFFER_SIZE 1024

using namespace kxemu::device;

static int open_socket_client(const std::string &ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        WARN("Failed to create socket.");
        return -1;
    }
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
        WARN("Invalid address.");
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        WARN("Connect failed.");
        perror("Connect failed");
        return -1;
    }
    return sockfd;
}

word_t Uart16650::read(word_t offset, word_t size, bool &valid) {
    if (size != 1) {
        WARN("uart read size " FMT_VARU64 " not supported.", size);
        valid = false;
        return -1;
    }

    valid = true;
    if (offset == 0) {
        if (lcr & 0x80) { // Divisor Latch Access Bit is set
            // LSB: Lower 8 bits of the divisor latch
            return lsb;
        } else {
            // RBR: Receiver Buffer Register
            if (queue.empty()) {
                return -1;
            }

            queueMtx.lock();
            uint8_t c = queue.front();
            queue.pop();
            if (queue.empty()) {
                lsr &= ~LSR_RX_READY;
            }
            queueMtx.unlock();
            
            // Clear Receiver Data Available Interrupt
            if (ier & 0x01 && (iir & 0b00111111) == 0b10) {
                iir = 0b11000000; // Clear Interrupt
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
    } else if (offset == 2) {
        // FCR: FIFO Control Register
        if (data & (1 << 1)) {
            // Clear Receive FIFO
            queueMtx.lock();
            while (!queue.empty()) {
                queue.pop();
            }
            lsr &= ~LSR_RX_READY;
            queueMtx.unlock();
        }

        switch ((data & 0b11000000) >> 6) {
            case 0b00: recvFIFOTriggerByteCount = 1; break;
            case 0b01: recvFIFOTriggerByteCount = 4; break;
            case 0b10: recvFIFOTriggerByteCount = 8; break;
            case 0b11: recvFIFOTriggerByteCount = 14; break;
        }

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
        return false;
    }
}

void Uart16650::update() {
    if (mode != Mode::SOCKET) return;

    static struct timeval timeout = {};
    static fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(recvSocket, &read_fds);

    int r = select(recvSocket + 1, &read_fds, NULL, NULL, &timeout);
    if (r == -1) {
        WARN("Failed to select socket.");
        return;
    }
    if (r == 0) {
        return;
    }

    char buffer[64];
    ssize_t n = ::read(recvSocket, buffer, sizeof(buffer));
    if (n <= 0) {
        WARN("Failed to receive data from socket.");
        mode = Mode::NONE;
        return;
    }

    for (int i = 0; i < n; i++) {
        recv_byte(buffer[i]);
    }
}

bool Uart16650::interrupt_pending() {
    return this->interrput;
}

void Uart16650::clear_interrupt() {
    this->interrput = false;
}

bool Uart16650::putch(uint8_t data) {
    std::lock_guard<std::mutex> lock(queueMtx);
    if (queue.size() >= BUFFER_SIZE) {
        return false;
    }
    queue.push(data);
    lsr |= LSR_RX_READY;
    return true;
}

void Uart16650::set_output_stream(std::ostream &os) {
    if (mode == Mode::SOCKET) {
        PANIC("Cannot change to use stream.");
    } 
    mode = Mode::STREAM;
    stream = &os;
    // For now, we assume that the stream is always ready to write
    lsr |= LSR_TX_READY;
}

bool Uart16650::open_socket(const std::string &ip, int port) {
    if (mode != Mode::NONE) {
        PANIC("Cannot change to use socket.");
        return false;
    }
    recvSocket = open_socket_client(ip, port);
    sendSocket = open_socket_client(ip, port + 1);
    if ((sendSocket < 0)) {
        WARN("Failed to open socket.");
        return false;
    }
    // uartSocketRunning = true;
    // recvThread = new std::thread(&Uart16650::recv_thread_loop, this);
    mode = Mode::SOCKET;
    lsr |= LSR_TX_READY;
    return true;
}

void Uart16650::recv_byte(uint8_t c) {
    std::lock_guard<std::mutex> lock(queueMtx);
    if (queue.size() >= BUFFER_SIZE) {
        WARN("uart buffer is full, drop data.");
        return;
    }
    queue.push(c);
    lsr |= LSR_RX_READY;

    if (queue.size() >= this->recvFIFOTriggerByteCount) {
        if (ier & 0x01) {
            iir = 0b10 | 0b11000000; // Interrupt Pending
            this->interrput = true;
            INFO("UART interrupt pending.");
        }
    }
}

void Uart16650::send_byte(uint8_t c) {
    std::lock_guard<std::mutex> lock(senderMtx);
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
        ssize_t n = send(sendSocket, &c, 1, 0);
        if (n <= 0) {
            WARN("Failed to send data to socket.");
        }
    }
}

const char *Uart16650::get_type_name() const {
    return "uart16650";
}

// void Uart16650::recv_thread_loop() {
//     struct timeval timeout;
//     timeout.tv_sec = 1;
//     timeout.tv_usec = 0;

//     fd_set read_fds;
    
//     uint8_t buffer[64];
//     while (uartSocketRunning) {
//         FD_ZERO(&read_fds);
//         FD_SET(recvSocket, &read_fds);

//         int r = select(recvSocket + 1, &read_fds, NULL, NULL, &timeout);

//         if (r == -1) {
//             WARN("Failed to select socket.");
//             break;
//         }
//         if (r == 0) {
//             continue;
//         }

//         ssize_t n = ::read(recvSocket, buffer, sizeof(buffer));
//         if (n <= 0) {
//             WARN("Failed to receive data from socket.");
//             break;
//         }

//         queueMtx.lock();
//         for (int i = 0; i < n; i++) {
//             queue.push(buffer[i]);
//         }
//         lsr |= LSR_RX_READY;
//         queueMtx.unlock();
//     }
//     // close socket
//     close(recvSocket);
//     close(sendSocket);
//     mode = Mode::NONE;
// }

Uart16650::~Uart16650() {
    // uartSocketRunning = false;
    // if (recvThread != nullptr) {
    //     recvThread->join();
    //     delete recvThread;
    // }
    close(recvSocket);
    close(sendSocket);
}
