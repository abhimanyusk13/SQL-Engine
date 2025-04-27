#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "Schema.h"
#include "Catalog.h"
#include "FileManager.h"
#include "BufferManager.h"
#include "TableHeap.h"
#include "BPlusTree.h"
#include "Expr.h"
#include "ExpressionEvaluator.h"
#include "LockManager.h"
#include "WALManager.h"

using PKIndex = BPlusTree<int32_t, RecordID>;

class StorageEngine {
public:
    StorageEngine(FileManager &fm,
                  BufferManager &bm,
                  Catalog &catalog,
                  LockManager &lockMgr,
                  WALManager &walMgr);

    void registerTable(const std::string &name,
                       const Schema &schema,
                       const std::string &dataFile,
                       const std::string &indexFile,
                       const std::string &pkColumn);

    RecordID insertRecord(const std::string &tableName,
                          const std::vector<std::string> &cols,
                          const std::vector<FieldValue> &vals,
                          int64_t txId);

    void deleteRecords(const std::string &tableName,
                       Expr *where,
                       int64_t txId);

    void updateRecords(const std::string &tableName,
                       const std::vector<std::pair<std::string, Expr*>> &assigns,
                       Expr *where,
                       int64_t txId);

    std::vector<FieldValue> getRecord(const std::string &tableName,
                                      const RecordID &rid);

    std::vector<RecordID> scanTable(const std::string &tableName,
                                    int64_t txId);

    // in StorageEngine.h (public API)
    /// Replay‐level calls from WAL recovery
    void redoInsert(const std::string &tableName,
        const RecordID &rid,
        const std::vector<FieldValue> &newValues);

    void redoDelete(const std::string &tableName,
        const RecordID &rid);

    void redoUpdate(const std::string &tableName,
        const RecordID &rid,
        const std::vector<FieldValue> &newValues);


private:
    struct TableData {
        std::unique_ptr<TableHeap> heap;
        std::unique_ptr<PKIndex>   index;
        int                         pkColIdx;
    };

    FileManager    &fm_;
    BufferManager  &bm_;
    Catalog        &catalog_;
    LockManager    &lockMgr_;
    WALManager     &walMgr_;
    std::unordered_map<std::string, TableData> tables_;
};
