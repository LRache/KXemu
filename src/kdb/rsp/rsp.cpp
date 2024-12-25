#include "kdb/rsp.h"
#include "debug.h"
#include "log.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define BUFFER_SIZE 1024

static int server;
static int client;

static bool start_tcp_server(int port) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);

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

    if ((client = accept(server, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        std::cout << strerror(errno) << std::endl;
        close(server);
        return false;
    }

    return true;
}

bool check_valid(const char *buffer, size_t n) {
    // empty
    if (n == 0) {
        return false;
    }

    // check header
    if (buffer[0] != '$') {
        return false;
    }

    // check sum
    char payload[BUFFER_SIZE];
    uint32_t checksum;
    sscanf(buffer, "$%s#%x", payload, &checksum);
    uint32_t sum = 0;
    for (int i = 0; payload[i] != '\0'; i++) {
        uint8_t c = payload[i];
        sum += c;
    }
    std::cout << sum % 256 << std::endl; 
    return sum % 256 == checksum;
}

void rsp::rsp_mainloop(int port) {
    NOT_IMPLEMENTED();
    
    if (!start_tcp_server(port)) {
        std::cout << "Failed to start TCP server." << std::endl;
        return;
    }

    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t n = read(client, buffer, BUFFER_SIZE);
        if (n < 0) {
            std::cout << strerror(errno);
            INFO("Break");
            break;
        }
        if (!check_valid(buffer, n)) {
            std::cout << buffer << std::endl;
            std::cout << "Invalid packet." << std::endl;
            continue;
        }
    }
    close(server);

    INFO("RSP server end");
}
