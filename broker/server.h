#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include <unordered_set>

class Server {
public:
    Server();
    void start();

private:
    void handleClient(int client_socket);
    std::string handleRequest(const std::string& request, int clientId);
    void subscribe(int clientId, const std::string& topic);
    void unsubscribe(int clientId, const std::string& topic);
    void publish(const std::string& topic, const std::string& message, const std::string& uuid);
    std::vector<std::string> getMessages(int clientId, const std::string& topic);
    void removeClient(int clientId);

    std::map<std::string, std::vector<int>> subscriptions;
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> messages; // <topic, <message, uuid>>
    std::unordered_set<std::string> processedUUIDs;
    std::mutex mtx;
    std::atomic<bool> running;
    std::map<int, std::map<std::string, size_t>> clientMessageIndices;
};

#endif // SERVER_H