// File: ConnectionManager.cpp
#include "ConnectionManager.h"
#include <iostream>

void ConnectionManager::init(int port) {
    // TODO: Bind socket, listen
    std::cout << "Listening on port " << port << "...\n";
}

void ConnectionManager::acceptConnections() {
    // TODO: Accept loop, hand off to sessions
}
