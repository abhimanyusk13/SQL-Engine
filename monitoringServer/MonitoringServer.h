#pragma once
#include <thread>
#include <string>

/// Simple HTTP server exposing MetricsManager at /metrics
class MonitoringServer {
public:
    /// port to listen on (defaults to 8080)
    explicit MonitoringServer(int port = 8080);
    ~MonitoringServer();

    /// Start the server in a detached thread
    void start();

private:
    int port_;
    int listenFd_;
    std::thread thread_;

    void run();
    void handleClient(int clientFd);
};
