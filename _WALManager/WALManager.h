// File: WALManager.h
#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "StorageEngine.h"    // for RecordID, FieldValue

/// Log‐record types
enum class LogOp { INSERT, DELETE, UPDATE, COMMIT, ABORT };

struct LogRecord {
    LogOp               op;
    int64_t             txId;
    std::string         table;
    RecordID            rid;
    std::vector<FieldValue> oldValues;  // for DELETE & UPDATE
    std::vector<FieldValue> newValues;  // for INSERT & UPDATE
};

class WALManager {
public:
    explicit WALManager(const std::string &logFilePath);
    ~WALManager();

    void logInsert(int64_t txId,
                   const std::string &tableName,
                   const RecordID &rid,
                   const std::vector<FieldValue> &newValues);

    void logDelete(int64_t txId,
                   const std::string &tableName,
                   const RecordID &rid,
                   const std::vector<FieldValue> &oldValues);

    void logUpdate(int64_t txId,
                   const std::string &tableName,
                   const RecordID &rid,
                   const std::vector<FieldValue> &oldValues,
                   const std::vector<FieldValue> &newValues);

    void logCommit(int64_t txId);
    void logAbort(int64_t txId);
    void flush();

    /// 1) REDO all committed ops in forward order
    /// 2) UNDO all uncommitted ops in reverse order
    /// 3) Truncate the log
    void recover(StorageEngine &storage);

private:
    std::mutex       latch_;
    std::string      logFile_;
    std::ofstream    logOut_;

    // parse helpers
    std::vector<std::string> split(const std::string &s, char delim);
    FieldValue  parseFV(const std::string &tok);
};
