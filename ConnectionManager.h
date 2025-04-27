#pragma once
#include <string>

class ConnectionManager {
public:
    // Initialize listener on given port
    void init(int port);
    // Accept new SQL connections
    void acceptConnections();
};
