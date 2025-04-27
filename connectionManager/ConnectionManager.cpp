// File: ConnectionManager.cpp
#include "ConnectionManager.h"
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>

ConnectionManager::ConnectionManager(QueryEngine &engine, int port)
    : engine_(engine), port_(port)
{
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) perror("socket"), exit(1);

    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenFd_, (sockaddr*)&addr, sizeof(addr)) < 0)
        perror("bind"), exit(1);
    if (listen(listenFd_, 10) < 0)
        perror("listen"), exit(1);

    std::cout << "Listening on port " << port_ << "...\n";
}

ConnectionManager::~ConnectionManager() {
    close(listenFd_);
}

void ConnectionManager::run() {
    while (true) {
        sockaddr_in cliAddr;
        socklen_t   len = sizeof(cliAddr);
        int clientFd = accept(listenFd_, (sockaddr*)&cliAddr, &len);
        if (clientFd < 0) {
            perror("accept");
            continue;
        }
        std::cout << "Client connected: "
                  << inet_ntoa(cliAddr.sin_addr) << ":" 
                  << ntohs(cliAddr.sin_port) << "\n";
        handleClient(clientFd);
        close(clientFd);
        std::cout << "Client disconnected\n";
    }
}

void ConnectionManager::handleClient(int clientFd) {
    while (true) {
        std::string sql = readStatement(clientFd);
        if (sql.empty()) break;  // client closed or error

        try {
            auto rows = engine_.executeQuery(sql);
            if (rows.empty()) {
                sendOk(clientFd);
            } else {
                sendResult(clientFd, rows);
            }
        } catch (const std::exception &e) {
            sendError(clientFd, e.what());
        }
    }
}

std::string ConnectionManager::readStatement(int clientFd) {
    std::string stmt;
    char buf[512];
    while (true) {
        ssize_t n = read(clientFd, buf, sizeof(buf));
        if (n <= 0) return "";  // EOF or error
        stmt.append(buf, buf + n);
        auto pos = stmt.find(';');
        if (pos != std::string::npos) {
            std::string out = stmt.substr(0, pos);
            // consume up to and including ';'
            stmt.erase(0, pos + 1);
            // trim whitespace
            size_t b = out.find_first_not_of(" \t\r\n");
            if (b==std::string::npos) return "";
            size_t e = out.find_last_not_of(" \t\r\n");
            return out.substr(b, e-b+1);
        }
    }
}

void ConnectionManager::sendOk(int clientFd) {
    const char *ok = "OK\n";
    write(clientFd, ok, strlen(ok));
}

void ConnectionManager::sendError(int clientFd, const std::string &msg) {
    std::string out = std::string("ERROR: ") + msg + "\n";
    write(clientFd, out.c_str(), out.size());
}

void ConnectionManager::sendResult(int clientFd,
                                   const std::vector<physical::Row> &rows) {
    for (auto &row : rows) {
        std::string line;
        for (size_t i = 0; i < row.size(); ++i) {
            line += fvToString(row[i]);
            if (i + 1 < row.size()) line += "\t";
        }
        line += "\n";
        write(clientFd, line.c_str(), line.size());
    }
}

std::string ConnectionManager::fvToString(const FieldValue &fv) {
    if (std::holds_alternative<int32_t>(fv)) {
        return std::to_string(std::get<int32_t>(fv));
    } else {
        // escape tabs/newlines if needed
        const auto &s = std::get<std::string>(fv);
        std::string out;
        for (char c : s) {
            if (c=='\t') out += "\\t";
            else if (c=='\n') out += "\\n";
            else out += c;
        }
        return out;
    }
}
