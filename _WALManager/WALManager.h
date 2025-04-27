#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include <unordered_set>

#include "StorageEngine.h"    // for RecordID, FieldValue

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

    /// Flush to disk
    void flush();

    /// On startup, replay or undo as needed, then truncate the log
    void recover(StorageEngine &storage);

private:
    std::mutex       latch_;
    std::string      logFile_;
    std::ofstream    logOut_;

    // Helpers for serialization
    std::string fvToString(const FieldValue &fv);
    FieldValue  stringToFv(const std::string &s);
    std::vector<std::string> split(const std::string &line, char delim);
};
