#include "publisher.h"
#include "../common/network.h"
#include "../common/message.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cerrno>
#include <cstring>
#include <uuid/uuid.h>

Publisher::Publisher(const std::string& serverAddress, int serverPort)
        : serverAddress(serverAddress), serverPort(serverPort) {
    std::srand(std::time(nullptr));
    clientId = std::rand();
}

void Publisher::publish(const std::string& topic, const std::string& message) {
    Message msg;
    msg.type = "PUBLISH";
    msg.topic = topic;
    msg.content = message;
    msg.clientId = clientId;
    msg.uuid = generateUUID();

    std::string response = sendRequest(msg.serialize());
    std::cout << "Server response: " << response << std::endl;
}

std::string Publisher::sendRequest(const std::string& request) {
    int sock = createConnection(serverAddress, serverPort);
    if (sock < 0) {
        std::cerr << "Failed to create connection: " << strerror(errno) << std::endl;
        return "";
    }

    if (sendMessage(sock, request) < 0) {
        std::cerr << "Failed to send message: " << strerror(errno) << std::endl;
        closeConnection(sock);
        return "";
    }

    std::string response = receiveMessage(sock);
    closeConnection(sock);

    return response;
}

std::string Publisher::generateUUID() {
    uuid_t newUuid;
    uuid_generate(newUuid);
    char uuidStr[37];
    uuid_unparse(newUuid, uuidStr);
    return std::string(uuidStr);
}