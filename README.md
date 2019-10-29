# TopicSubscriber

CLIENT/SERVER APPLICATION FOR SUBSCRIPTIONS MANAGEMENT

UDP clients send messages with different topics to TCP clients through the server. TCP clients subscribe to topics, receiving messages even after reconnection. Messages for offline clients are stored in C++ hashmaps with Time-to-Live fields. Multiplexing allows multiple TCP clients to be connected at the same time.
