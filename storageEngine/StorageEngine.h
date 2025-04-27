// File: StorageEngine.h
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "Schema.h"
#include "Record.h"
#include "BufferManager.h"
#include "FileManager.h"
#include "TableHeap.h"
#include "BPlusTree.h"

struct RecordID;  // Defined in TableHeap.h

class StorageEngine {
public:
    StorageEngine(FileManager &fm, BufferManager &bm);

    // Register a table with data file, index file, schema, and primary key column
    void registerTable(const std::string &tableName,
                       const Schema &schema,
                       const std::string &dataFile,
                       const std::string &indexFile,
                       const std::string &primaryKeyColumn);

    // Insert record and update primary-key index
    RecordID insertRecord(const std::string &tableName,
                          const std::vector<FieldValue> &values);

    // Delete record by primary key
    bool deleteByKey(const std::string &tableName, int32_t key);

    // Update record by primary key (PK cannot change)
    bool updateByKey(const std::string &tableName,
                     int32_t key,
                     const std::vector<FieldValue> &values);

    // Find RecordID by primary key
    bool findByKey(const std::string &tableName,
                   int32_t key,
                   RecordID &outRid) const;

    // Scan all records (returns RecordIDs)
    std::vector<RecordID> scanTable(const std::string &tableName) const;
    std::vector<FieldValue> fetchRecord(const std::string &tableName, const RecordID &rid) const;

private:
    struct TableInfo {
        Schema schema;
        std::unique_ptr<TableHeap> heap;
        std::unique_ptr<BPlusTree<int32_t, RecordID>> index;
        int pkColIdx;
    };

    FileManager &fm_;
    BufferManager &bm_;
    std::unordered_map<std::string, TableInfo> tables_;
};