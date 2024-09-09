#ifndef NETWORK_H
#define NETWORK_H

#include <string>

int createServerSocket(int port);
int acceptConnection(int server_fd);
int createConnection(const std::string& address, int port);
int sendMessage(int sock, const std::string& message);
std::string receiveMessage(int sock);
void closeConnection(int sock);

#endif // NETWORK_H