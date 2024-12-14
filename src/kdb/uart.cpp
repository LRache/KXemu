#include "device/uart.h"
#include "isa/word.h"
#include "kdb/kdb.h"

std::vector<Uart16650 *> kdb::uart::list;

static bool add_uart_map(word_t base, Uart16650 *uart) {
    if (!kdb::memory->add_memory_map("uart", base, UART_LENGTH, uart)) {
        // Add memory map failed, free
        delete uart;
        return false;
    }
    kdb::uart::list.push_back(uart);
    return true; 
}

bool kdb::uart::add(word_t base, std::ostream &os) {
    Uart16650 *uart = new Uart16650();
    if (!add_uart_map(base, uart)) {
        return false;
    }
    uart->set_output_stream(os);
    return true;
}

bool kdb::uart::add(word_t base, const std::string &ip, int port) {
    Uart16650 *uart = new Uart16650();
    if (!add_uart_map(base, uart)) {
        return false;
    }
    uart->open_socket(ip, port);
    return true;
}
