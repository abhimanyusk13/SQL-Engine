// File: ConnectionManager.h
#pragma once

#include <string>
#include <vector>
#include <sys/socket.h>
#include "QueryEngine.h"

namespace physical { using Row = std::vector<FieldValue>; }

class ConnectionManager {
public:
    /// engine: your SQL engine;
    /// port: TCP port to listen on (e.g. 5432)
    ConnectionManager(QueryEngine &engine, int port);
    ~ConnectionManager();

    /// Starts listening and serving clients (blocks).
    void run();

private:
    QueryEngine &engine_;
    int           port_;
    int           listenFd_;

    /// Handle a single client until they disconnect.
    void handleClient(int clientFd);

    /// Reads up to the next semicolon-terminated SQL statement.
    /// Returns empty string on EOF or error.
    std::string readStatement(int clientFd);

    /// Send back a successful DML/DDL ack.
    void sendOk(int clientFd);

    /// Send back query results (row-by-row, tab-separated).
    void sendResult(int clientFd, const std::vector<physical::Row> &rows);

    /// Send back an error message.
    void sendError(int clientFd, const std::string &msg);

    /// Convert a FieldValue to a string.
    std::string fvToString(const FieldValue &fv);
};
