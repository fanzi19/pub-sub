#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

struct Message {
    std::string type;
    std::string topic;
    std::string content;
    int clientId;
    std::string uuid;

    std::string serialize() const;
    static Message deserialize(const std::string& data);
};

#endif // MESSAGE_H