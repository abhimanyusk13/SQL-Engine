#include "MonitoringServer.h"
#include "MetricsManager.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

MonitoringServer::MonitoringServer(int port)
  : port_(port)
{
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        perror("metrics socket");
        exit(1);
    }
    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listenFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("metrics bind");
        exit(1);
    }
    if (listen(listenFd_, 5) < 0) {
        perror("metrics listen");
        exit(1);
    }
    std::cout << "Metrics server listening on port " << port_ << "\n";
}

MonitoringServer::~MonitoringServer() {
    close(listenFd_);
    // Note: thread_ is detached
}

void MonitoringServer::start() {
    thread_ = std::thread([this]{ run(); });
    thread_.detach();
}

void MonitoringServer::run() {
    while (true) {
        sockaddr_in cli{};
        socklen_t len = sizeof(cli);
        int clientFd = accept(listenFd_, (sockaddr*)&cli, &len);
        if (clientFd < 0) continue;
        std::thread(&MonitoringServer::handleClient, this, clientFd).detach();
    }
}

void MonitoringServer::handleClient(int clientFd) {
    // Read first line (we ignore the full request)
    char buf[1024];
    ssize_t n = read(clientFd, buf, sizeof(buf));
    if (n <= 0) { close(clientFd); return; }

    // Simple check: GET /metrics
    std::string req(buf, buf + n);
    if (req.rfind("GET /metrics", 0) == 0) {
        auto body = MetricsManager::instance().getMetrics();
        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: text/plain\r\n"
            << "Content-Length: " << body.size() << "\r\n"
            << "Connection: close\r\n\r\n"
            << body;
        auto resp = oss.str();
        write(clientFd, resp.data(), resp.size());
    } else {
        const char *notf = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
        write(clientFd, notf, strlen(notf));
    }
    close(clientFd);
}
