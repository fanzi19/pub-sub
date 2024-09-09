#include "subscriber.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <sstream>

Subscriber::Subscriber(const std::string& host, int port) : host(host), port(port), socket(-1) {}

Subscriber::~Subscriber() {
    disconnect();
}

bool Subscriber::connect() {
    socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket == -1) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &(serverAddress.sin_addr)) <= 0) {
        std::cerr << "Invalid address: " << strerror(errno) << std::endl;
        close(socket);
        socket = -1;
        return false;
    }

    if (::connect(socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to connect to server: " << strerror(errno) << std::endl;
        close(socket);
        socket = -1;
        return false;
    }

    // Set socket to non-blocking mode
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Failed to get socket flags: " << strerror(errno) << std::endl;
        close(socket);
        socket = -1;
        return false;
    }
    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set socket to non-blocking mode: " << strerror(errno) << std::endl;
        close(socket);
        socket = -1;
        return false;
    }

    return true;
}

void Subscriber::disconnect() {
    if (socket != -1) {
        close(socket);
        socket = -1;
    }
}

bool Subscriber::subscribe(const std::string& topic) {
    std::cout << "Attempting to subscribe to topic: " << topic << std::endl;

    std::string request = "SUBSCRIBE:" + topic;
    if (send(socket, request.c_str(), request.length(), 0) == -1) {
        std::cerr << "Failed to send subscription request: " << strerror(errno) << std::endl;
        return false;
    }

    std::cout << "Subscription request sent, waiting for response..." << std::endl;

    // Wait for response with timeout
    fd_set readfds;
    struct timeval tv;
    FD_ZERO(&readfds);
    FD_SET(socket, &readfds);
    tv.tv_sec = 5;  // 5 seconds timeout
    tv.tv_usec = 0;

    int select_result = select(socket + 1, &readfds, NULL, NULL, &tv);
    if (select_result == -1) {
        std::cerr << "Select error: " << strerror(errno) << std::endl;
        return false;
    } else if (select_result == 0) {
        std::cerr << "Timeout waiting for server response" << std::endl;
        return false;
    }

    char buffer[1024];
    int bytes_received = recv(socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received == -1) {
        std::cerr << "Error receiving server response: " << strerror(errno) << std::endl;
        return false;
    } else if (bytes_received == 0) {
        std::cerr << "Server closed the connection" << std::endl;
        return false;
    }

    buffer[bytes_received] = '\0';
    std::string response(buffer);
    std::cout << "Received response: " << response << std::endl;

    if (response.find("SUBSCRIBED:" + topic) != std::string::npos) {
        std::cout << "Successfully subscribed to topic: " << topic << std::endl;
        return true;
    } else {
        std::cout << "Failed to subscribe to topic: " << topic << std::endl;
        return false;
    }
}

bool Subscriber::unsubscribe(const std::string& topic) {
    std::string message = "UNSUBSCRIBE " + topic + "\n";
    if (send(socket, message.c_str(), message.length(), 0) == -1) {
        std::cerr << "Failed to send unsubscribe message: " << strerror(errno) << std::endl;
        return false;
    }

    char buffer[1024] = {0};
    int bytesRead = 0;
    int totalBytesRead = 0;

    fd_set readfds;
    struct timeval tv;
    tv.tv_sec = 5;  // 5 seconds timeout
    tv.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(socket, &readfds);

    int selectResult = select(socket + 1, &readfds, NULL, NULL, &tv);
    if (selectResult == -1) {
        std::cerr << "Select failed: " << strerror(errno) << std::endl;
        return false;
    } else if (selectResult == 0) {
        std::cerr << "Timeout waiting for server response" << std::endl;
        return false;
    }

    while ((bytesRead = recv(socket, buffer + totalBytesRead, sizeof(buffer) - totalBytesRead - 1, 0)) > 0) {
        totalBytesRead += bytesRead;
        if (totalBytesRead > 0 && buffer[totalBytesRead - 1] == '\n') {
            break;
        }
    }

    if (bytesRead == -1 && errno != EWOULDBLOCK && errno != EAGAIN) {
        std::cerr << "Failed to receive server response: " << strerror(errno) << std::endl;
        return false;
    }

    buffer[totalBytesRead] = '\0';
    std::string response(buffer);

    // Update this part to handle the "UNSUBSCRIBED:topic" response
    if (response == "OK\n" || response == "UNSUBSCRIBED:" + topic + "\n") {
        return true;
    } else {
        std::cerr << "Unexpected server response: " << response << std::endl;
        return false;
    }
}

void Subscriber::processIncomingMessage(const std::string& serializedMessage) {
    std::cout << "Processing message: " << serializedMessage << std::endl;  // Debug output
    try {
        Message msg = Message::deserialize(serializedMessage);
        if (msg.type == "PUBLISH") {
            std::lock_guard<std::mutex> lock(messageMutex);
            receivedMessages[msg.topic].push_back(msg);
            std::cout << "Stored message for topic '" << msg.topic
                      << "': content='" << msg.content
                      << "', clientId=" << msg.clientId
                      << ", uuid=" << msg.uuid << std::endl;  // Debug output
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
    }
}

bool Subscriber::getNextMessage(const std::string& topic, Message& message) {
    char buffer[1024];
    int bytesRead;

    // Set socket to non-blocking mode
    int flags = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);

    bytesRead = recv(socket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string msg(buffer);
        std::cout << "Received raw message: " << msg << std::endl;  // Debug output

        std::istringstream iss(msg);
        std::string line;
        while (std::getline(iss, line)) {
            processIncomingMessage(line);
        }
    } else if (bytesRead == -1) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            std::cerr << "Error receiving message: " << strerror(errno) << std::endl;
        }
    }

    std::lock_guard<std::mutex> lock(messageMutex);
    auto& messages = receivedMessages[topic];
    if (!messages.empty()) {
        message = messages.front();
        messages.erase(messages.begin());
        return true;
    }

    return false;
}