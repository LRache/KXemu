#include "utils/gdb-rsp.h"
#include "debug.h"

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>

RSP::RSP() {
    this->server = -1;
    this->client = -1;
}

bool RSP::check_valid(const char *buffer, size_t n) {
    if (n == 0) {
        return false;
    }

    if (buffer[0] != '$') {
        return false;
    }

    // check sum
    

    return true;
}

bool RSP::open_rsp(int port) {
    if ((server = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        return false;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server, (struct sockaddr *)&address, sizeof(address)) < 0) {
        return false;
    }

    if (listen(server, 1) < 0) {
        return false;
    }

    return true;
}

bool RSP::wait_client() {
    SELF_PROTECT(this->server != -1, "Server is not initialized.")

    int addrlen = sizeof(address);
    if ((client = accept(server, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        std::cout << strerror(errno) << std::endl;
        close(server);
        return false;
    }

    return true;
}

int RSP::next_command(std::vector<std::string> &args, bool blocked) {

    return 0;
}
