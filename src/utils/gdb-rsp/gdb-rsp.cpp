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

#define BUFFER_SIZE 1024

static unsigned int hexchar_to_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else {
        return 0;
    }
}


static bool check_valid(const char *buffer, size_t n) {
    if (n < 4) {
        return false;
    }

    if (buffer[0] != '$') {
        return false;
    }

    // check sum
    unsigned int checksum = (hexchar_to_digit(buffer[n - 2]) << 4) + hexchar_to_digit(buffer[n - 1]);
    unsigned int sum = 0;
    std::cout << "payload: ";
    for (size_t i = 1; i < n - 3; i++) {
        sum += buffer[i];
    }

    return (sum & 0xff) == checksum;
}

RSP::RSP() {
    this->server = -1;
    this->client = -1;
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
    char c;
    int n = read(client, &c, 1);
    if (n < 0) {
        close(client);
        client = -1;
        return false;
    }

    return true;
}

int RSP::next_command(std::vector<std::string> &, bool blocked) {
    SELF_PROTECT(this->client != -1, "Client is not initialized.")

    if (commandQueue.empty()) {
        if (!blocked) {
            return CommandType::Empty;
        }
        
        char buffer[BUFFER_SIZE];
        int n = read(client, buffer, BUFFER_SIZE);
        if (n <= 0) {
            return CommandType::Error;
        }
        buffer[n] = '\0';
        if (!check_valid(buffer, n)) {
            std::cout << "Invalid command: " << buffer << std::endl;
            return CommandType::Error;
        }
        std::cout << buffer << std::endl;
    }
    return 0;
}

RSP::~RSP() {
    if (client != -1) {
        close(client);
    }
    if (server != -1) {
        close(server);
    }
}
