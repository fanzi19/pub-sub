#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <set>
#include "../common/message.h" // Include the Message header

class Subscriber {
public:
    Subscriber(const std::string& host, int port);
    ~Subscriber();

    bool connect();
    void disconnect();
    bool subscribe(const std::string& topic);
    bool unsubscribe(const std::string& topic);
    bool getNextMessage(const std::string& topic, Message& message);

private:
    std::string host;
    int port;
    int socket;
    std::map<std::string, std::vector<Message>> receivedMessages;
    std::mutex messageMutex;
    std::set<std::string> processedUUIDs;
    std::mutex uuidMutex;

    void processIncomingMessage(const std::string& serializedMessage);
};