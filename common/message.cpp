#include "message.h"
#include <sstream>

std::string Message::serialize() const {
    std::ostringstream oss;
    oss << type << ":" << topic << ":" << content << ":" << clientId << ":" << uuid;
    return oss.str();
}

Message Message::deserialize(const std::string& data) {
    Message msg;
    std::istringstream iss(data);

    std::getline(iss, msg.type, ':');
    std::getline(iss, msg.topic, ':');
    std::getline(iss, msg.content, ':');
    iss >> msg.clientId;
    iss.ignore(); // Ignore the space after clientId
    std::getline(iss, msg.uuid);

    return msg;
}