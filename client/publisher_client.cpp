#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include "../common/message.h" // Include the Message header

#define PORT 8080

std::string generate_uuid() {
    uuid_t uuid;
    char uuid_str[37];
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, uuid_str);
    return std::string(uuid_str);
}

void publish(int sock) {
    std::string topic, content;
    std::cout << "Enter topic: ";
    std::getline(std::cin, topic);
    std::cout << "Enter message: ";
    std::getline(std::cin, content);

    Message msg;
    msg.type = "PUBLISH";
    msg.topic = topic;
    msg.content = content;
    msg.clientId = sock; // Using socket as clientId for simplicity
    msg.uuid = generate_uuid();

    std::string serialized_msg = msg.serialize();
    send(sock, serialized_msg.c_str(), serialized_msg.length(), 0);

    char buffer[1024] = {0};
    int valread = read(sock, buffer, 1024);
    std::cout << "Server response: " << buffer << std::endl;
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Connection Failed" << std::endl;
        return -1;
    }

    std::string command;
    while (true) {
        std::cout << "Enter command (publish/quit): ";
        std::getline(std::cin, command);

        if (command == "publish") {
            publish(sock);
        } else if (command == "quit") {
            break;
        } else {
            std::cout << "Invalid command. Please enter 'publish' or 'quit'." << std::endl;
        }
    }

    close(sock);
    return 0;
}