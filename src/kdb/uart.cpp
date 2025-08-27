#include "device/uart.hpp"
#include "kdb/kdb.hpp"

#include <string>

using namespace kxemu;
using kxemu::device::Uart16650;

std::vector<Uart16650 *> kdb::uart::list;

static bool add_uart_map(unsigned int id, kdb::word_t base, Uart16650 *uart) {
    if (!kdb::bus->add_mmio_map(id, base, UART_LENGTH, uart)) {
        // Add memory map failed, free
        delete uart;
        return false;
    }
    kdb::uart::list.push_back(uart);
    return true; 
}

bool kdb::uart::add(unsigned int id, word_t base, std::ostream &os) {
    Uart16650 *uart = new Uart16650();
    if (!add_uart_map(id, base, uart)) {
        return false;
    }
    uart->set_output_stream(os);
    return true;
}

bool kdb::uart::add(unsigned int id, word_t base, const std::string &ip, int port) {
    Uart16650 *uart = new Uart16650();
    if (!uart->open_socket(ip, port)) {
        delete uart;
        return false;
    }
    if (!add_uart_map(id, base, uart)) {
        return false;
    }
    device::add(uart);
    return true;
}

bool kdb::uart::puts(std::size_t index, const std::string &s) {
    if (index >= kdb::uart::list.size()) {
        return false;
    }
    for (char c : s) {
        kdb::uart::list[index]->putch(c);
    }
    return true;
}

void kdb::uart::deinit() {
    kdb::uart::list.clear();
}
