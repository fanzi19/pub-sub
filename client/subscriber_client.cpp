#include "client_api/subscriber.h"
#include "../common/message.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <map>
#include <mutex>
#include <queue>
#include <condition_variable>

std::atomic<bool> running(true);
std::map<std::string, std::atomic<bool>> pollThreadRunning;
std::mutex pollThreadsMutex;

std::mutex outputMutex;
std::queue<std::string> outputQueue;
std::condition_variable outputCV;

void queueOutput(const std::string& message) {
    std::lock_guard<std::mutex> lock(outputMutex);
    outputQueue.push(message);
    outputCV.notify_one();
}

void outputThread() {
    std::string lastPrompt;
    while (running) {
        std::unique_lock<std::mutex> lock(outputMutex);
        outputCV.wait(lock, [] { return !outputQueue.empty() || !running; });

        while (!outputQueue.empty()) {
            std::string message = outputQueue.front();
            outputQueue.pop();

            if (message == lastPrompt) {
                std::cout << std::endl;
            }

            std::cout << message << std::flush;

            if (message == "Enter command (subscribe/unsubscribe/quit): ") {
                lastPrompt = message;
            }
        }
    }
}

void pollMessages(Subscriber& subscriber, const std::string& topic) {
    queueOutput("\nStarted polling thread for topic: " + topic + "\n");
    queueOutput("Enter command (subscribe/unsubscribe/quit): ");
    while (pollThreadRunning[topic]) {
        Message message;
        if (subscriber.getNextMessage(topic, message)) {
            queueOutput("\nReceived message on topic '" + topic + "':\n" +
                        "  Content: " + message.content + "\n" +
                        "  Client ID: " + std::to_string(message.clientId) + "\n" +
                        "  UUID: " + message.uuid + "\n");
            queueOutput("Enter command (subscribe/unsubscribe/quit): ");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void handleCommands(Subscriber& subscriber) {
    std::string command, topic;
    std::map<std::string, std::thread> pollThreads;

    while (running) {
        queueOutput("Enter command (subscribe/unsubscribe/quit): ");
        std::cin >> command;

        if (command == "quit") {
            running = false;
            break;
        } else if (command == "subscribe" || command == "unsubscribe") {
            queueOutput("Enter topic: ");
            std::cin >> topic;

            if (command == "subscribe") {
                if (subscriber.subscribe(topic)) {
                    queueOutput("Successfully subscribed to topic: " + topic + "\n");
                    std::lock_guard<std::mutex> lock(pollThreadsMutex);
                    if (pollThreads.find(topic) == pollThreads.end()) {
                        pollThreadRunning[topic] = true;
                        pollThreads[topic] = std::thread(pollMessages, std::ref(subscriber), topic);
                        queueOutput("Created polling thread for topic: " + topic + "\n");
                    }
                } else {
                    queueOutput("Failed to subscribe to topic: " + topic + "\n");
                }
            } else { // unsubscribe
                if (subscriber.unsubscribe(topic)) {
                    queueOutput("Successfully unsubscribed from topic: " + topic + "\n");
                    std::lock_guard<std::mutex> lock(pollThreadsMutex);
                    if (pollThreads.find(topic) != pollThreads.end()) {
                        pollThreadRunning[topic] = false;
                        pollThreads[topic].join();
                        pollThreads.erase(topic);
                        queueOutput("Removed polling thread for topic: " + topic + "\n");
                    }
                } else {
                    queueOutput("Failed to unsubscribe from topic: " + topic + "\n");
                }
            }
        } else {
            queueOutput("Invalid command. Please use 'subscribe', 'unsubscribe', or 'quit'.\n");
        }
    }

    // Clean up remaining threads
    for (auto& pair : pollThreads) {
        pollThreadRunning[pair.first] = false;
        pair.second.join();
    }
}

int main() {
    Subscriber subscriber("127.0.0.1", 8080);

    if (!subscriber.connect()) {
        std::cout << "Failed to connect to the server." << std::endl;
        return 1;
    }

    std::cout << "Connected to 127.0.0.1:8080" << std::endl;

    std::thread outputThreadHandle(outputThread);
    std::thread commandThread(handleCommands, std::ref(subscriber));

    // Wait for the command thread to finish
    commandThread.join();

    // Signal the output thread to finish and wait for it
    running = false;
    outputCV.notify_one();
    outputThreadHandle.join();

    subscriber.disconnect();
    return 0;
}