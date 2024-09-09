#ifndef PUBLISHER_H
#define PUBLISHER_H

#include <string>
#include <uuid/uuid.h>

class Publisher {
public:
    Publisher(const std::string& serverAddress, int serverPort);
    void publish(const std::string& topic, const std::string& message);

private:
    std::string serverAddress;
    int serverPort;
    int clientId;
    uuid_t uuid;

    std::string sendRequest(const std::string& request);
    std::string generateUUID();
};

#endif // PUBLISHER_H