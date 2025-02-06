#ifndef __KXEMU_UTILS_TCP_SERVER_H__
#define __KXEMU_UTILS_TCP_SERVER_H__

#include <cstddef>
#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>

class TCPServer {
public:
    TCPServer();
    ~TCPServer();
    
    bool start(const std::string &ip, int port);
    int accept_connection();
    ssize_t receive(int client, uint8_t *buffer, std::size_t bufferSize);
    ssize_t send(int client, const uint8_t *buffer, std::size_t bufferSize);
    void close_connection(int client);
    void stop();
private:
    int serverSocket;
    struct sockaddr_in address;
    bool isRunning;
};

#endif
