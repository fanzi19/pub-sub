#include "network.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

int createServerSocket(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        return -1;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        return -1;
    }

    if (listen(server_fd, 3) < 0) {
        return -1;
    }

    return server_fd;
}

int acceptConnection(int server_fd) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    return new_socket;
}

int createConnection(const std::string& address, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, address.c_str(), &serv_addr.sin_addr) <= 0) {
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        return -1;
    }

    return sock;
}

int sendMessage(int sock, const std::string& message) {
    return send(sock, message.c_str(), message.length(), 0);
}

std::string receiveMessage(int sock) {
    char buffer[1024] = {0};
    int valread = read(sock, buffer, 1024);
    if (valread < 0) {
        return "";
    }
    return std::string(buffer, valread);
}

void closeConnection(int sock) {
    close(sock);
}