#include "server.h"
#include "../common/message.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>

#define PORT 8080
#define BUFFER_SIZE 1024

Server::Server() : running(true) {}

void Server::start() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation error: " << strerror(errno) << std::endl;
        return;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "Setsockopt error: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    std::cout << "Server listening on 0.0.0.0:" << PORT << std::endl;

    std::vector<pollfd> fds;
    fds.push_back({server_fd, POLLIN, 0});

    while(running) {
        int poll_count = poll(fds.data(), fds.size(), -1);

        if (poll_count == -1) {
            std::cerr << "Poll failed: " << strerror(errno) << std::endl;
            continue;
        }

        for (size_t i = 0; i < fds.size(); i++) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == server_fd) {
                    int client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                    if (client_socket < 0) {
                        std::cerr << "Accept failed: " << strerror(errno) << std::endl;
                    } else {
                        std::cout << "New connection accepted" << std::endl;
                        fds.push_back({client_socket, POLLIN, 0});
                    }
                } else {
                    handleClient(fds[i].fd);
                }
            }
        }

        fds.erase(std::remove_if(fds.begin() + 1, fds.end(),
                                 [this](const pollfd& pfd) {
                                     if (pfd.fd == -1) {
                                         removeClient(pfd.fd);
                                         return true;
                                     }
                                     return false;
                                 }),
                  fds.end());
    }

    for (const auto& fd : fds) {
        if (fd.fd != server_fd) {
            close(fd.fd);
        }
    }
    close(server_fd);
}

void Server::handleClient(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    int valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (valread <= 0) {
        if (valread == 0) {
            std::cout << "Client disconnected" << std::endl;
        } else {
            std::cerr << "Recv failed: " << strerror(errno) << std::endl;
        }
        close(client_socket);
        removeClient(client_socket);
        return;
    }

    std::string request(buffer);
    std::string response = handleRequest(request, client_socket);
    send(client_socket, response.c_str(), response.length(), 0);
}

std::string Server::handleRequest(const std::string& request, int clientId) {
    Message msg = Message::deserialize(request);

    std::cout << "Received request: " << request << std::endl;

    std::string response;

    if (msg.type == "SUBSCRIBE") {
        subscribe(clientId, msg.topic);
        response = "SUBSCRIBED:" + msg.topic + "\n";
    } else if (msg.type == "UNSUBSCRIBE") {
        unsubscribe(clientId, msg.topic);
        response = "UNSUBSCRIBED:" + msg.topic + "\n";
    } else if (msg.type == "PUBLISH") {
        publish(msg.topic, msg.content, msg.uuid);
        response = "PUBLISHED:" + msg.topic + "\n";
    } else if (msg.type == "GET_MESSAGES") {
        std::vector<std::string> clientMessages = getMessages(clientId, msg.topic);
        if (clientMessages.empty()) {
            response = "NO_MESSAGES\n";
        } else {
            for (const auto& cmsg : clientMessages) {
                response += "MESSAGE:" + msg.topic + ":" + cmsg + "\n";
            }
        }
    } else {
        response = "INVALID_COMMAND\n";
    }

    // Print out the subscriptions after handling the request
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "Current subscriptions:" << std::endl;
        for (const auto& sub : subscriptions) {
            std::cout << "Topic: " << sub.first << ", Subscribers: ";
            if (sub.second.empty()) {
                std::cout << "None";
            } else {
                for (size_t i = 0; i < sub.second.size(); ++i) {
                    std::cout << sub.second[i];
                    if (i < sub.second.size() - 1) {
                        std::cout << ", ";
                    }
                }
            }
            std::cout << std::endl;
        }
    }

    return response;
}

void Server::subscribe(int clientId, const std::string& topic) {
    std::lock_guard<std::mutex> lock(mtx);
    auto& subs = subscriptions[topic];
    if (std::find(subs.begin(), subs.end(), clientId) == subs.end()) {
        subs.push_back(clientId);
    }
}

void Server::unsubscribe(int clientId, const std::string& topic) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = subscriptions.find(topic);
    if (it != subscriptions.end()) {
        auto& subs = it->second;
        subs.erase(std::remove(subs.begin(), subs.end(), clientId), subs.end());
        if (subs.empty()) {
            subscriptions.erase(it);
        }
    }
}

void Server::publish(const std::string& topic, const std::string& message, const std::string& uuid) {
    std::lock_guard<std::mutex> lock(mtx);

    // Check if the message with this UUID has already been processed
    if (processedUUIDs.find(uuid) != processedUUIDs.end()) {
        std::cout << "Duplicate message with UUID: " << uuid << " ignored." << std::endl;
        return;
    }

    // Add the UUID to the set of processed UUIDs
    processedUUIDs.insert(uuid);

    messages[topic].push_back(std::make_pair(message, uuid));
    for (int subscriber : subscriptions[topic]) {
        std::string notification = "MESSAGE:" + topic + ":" + message + ":" + uuid + "\n";
        send(subscriber, notification.c_str(), notification.length(), 0);
    }
}

std::vector<std::string> Server::getMessages(int clientId, const std::string& topic) {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::string> newMessages;

    size_t& lastReadIndex = clientMessageIndices[clientId][topic];
    size_t currentSize = messages[topic].size();

    for (size_t i = lastReadIndex; i < currentSize; ++i) {
        const auto& pair = messages[topic][i];
        newMessages.push_back(pair.first + ":" + pair.second); // message:uuid
    }

    lastReadIndex = currentSize;

    // Only remove messages that have been read by all subscribed clients
    bool canRemoveMessages = true;
    for (const auto& clientIndices : clientMessageIndices) {
        if (clientIndices.second.count(topic) > 0 && clientIndices.second.at(topic) < currentSize) {
            canRemoveMessages = false;
            break;
        }
    }

    if (canRemoveMessages) {
        messages[topic].clear();
        for (auto& clientIndices : clientMessageIndices) {
            clientIndices.second[topic] = 0;
        }
    }

    return newMessages;
}

void Server::removeClient(int clientId) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& pair : subscriptions) {
        pair.second.erase(std::remove(pair.second.begin(), pair.second.end(), clientId), pair.second.end());
    }
    clientMessageIndices.erase(clientId); // Remove the client's message indices
}
