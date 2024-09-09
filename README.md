Overview
This project implements a Publish-Subscribe (Pub-Sub) system using C++, where publishers send messages to a broker, and subscribers receive messages from specific topics. Each message in the system is uniquely identified using a UUID to ensure accurate processing, including deduplication and tracking.

The broker manages message routing between publishers and subscribers, ensuring that each message is delivered to the correct topic subscribers.

Components
1. Broker
Main Files: main.cpp, server.cpp, server.h
The broker acts as a central entity that facilitates message delivery between publishers and subscribers.
It listens for incoming publisher messages and subscriber requests, and then routes messages to the appropriate subscribers.
The server employs polling to manage multiple client connections concurrently.
2. Client
Publisher Client: publisher_client.cpp
A client program that sends messages to the broker on specific topics.
It interacts with the broker via a network connection and uses a UUID to identify and track each message.
Subscriber Client: subscriber_client.cpp
A client program that subscribes to specific topics and receives messages.
The subscriber sends subscription requests to the broker and receives messages routed from the corresponding publisher.
3. Client API
Publisher API: publisher.cpp, publisher.h
Provides functions for publishers to interact with the broker, such as sending messages, setting topics, and handling UUIDs.
Subscriber API: subscriber.cpp, subscriber.h
Provides functions for subscribers to connect to the broker, subscribe to topics, and receive messages.
4. Common Utilities
Message and UUID Handling: message.cpp, message.h

Defines the structure of messages, which includes a UUID to uniquely identify each message.
Ensures that each message sent by a publisher can be tracked and deduplicated by both the broker and subscribers.
Networking Utilities: network.cpp, network.h

Implements socket communication logic for client-server interaction.
Handles setting up sockets, binding them to IP addresses and ports, and managing incoming/outgoing data.
Key Features
UUID-based Message Identification:

Each message from the publisher is tagged with a UUID to ensure that the broker and subscribers can uniquely identify it.
This helps in deduplication and accurate tracking of messages between publishers and subscribers.
Polling for Concurrency:

The broker uses a polling mechanism to handle multiple client connections (publishers and subscribers) at once.
This allows the system to scale effectively with multiple publishers and subscribers.
