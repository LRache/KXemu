#include "utils/tcp-server.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unistd.h>

TCPServer::TCPServer() {
    std::memset(&address, 0, sizeof(address));
}

bool TCPServer::start(const std::string &ip, int port) {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        return false;
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        close(serverSocket);
        return false;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // 监听所有接口
    address.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(serverSocket);
        return false;
    }

    if (listen(serverSocket, 3) < 0) {
        close(serverSocket);
        return false;
    }

    isRunning = true;
    return true;
}

int TCPServer::accept_connection() {
    if (!isRunning) {
        return -1;
    }

    struct sockaddr_in clientAddress;
    socklen_t clientAddrLen = sizeof(clientAddress);
    int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddrLen);
    if (clientSocket < 0) {
        perror("接受连接失败");
        return -1;
    }

    return clientSocket;
}

ssize_t TCPServer::receive(int client, uint8_t *buffer, std::size_t bufferSize) {
    ssize_t length = read(client, buffer, bufferSize);
    return length;
}

ssize_t TCPServer::send(int client, const uint8_t *buffer, std::size_t bufferSize) {
    ssize_t length = write(client, buffer, bufferSize);
    return length;
}

void TCPServer::close_connection(int client) {
    close(client);
}

void TCPServer::stop() {
    if (isRunning) {
        close(serverSocket);
        isRunning = false;
    }
}

TCPServer::~TCPServer() {
    stop();
}
