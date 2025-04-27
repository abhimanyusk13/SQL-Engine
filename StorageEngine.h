// File: StorageEngine.h
#pragma once
#include <string>
#include "BPlusTree.h"
#include "BufferManager.h"
#include "Record.h"

class StorageEngine {
public:
    // Create or open a table
    void openTable(const std::string &name);
    // Insert record to table
    void insertRecord(const std::string &table, const Record &rec);
    // Scan all records or by key range
    std::vector<Record> scanTable(const std::string &table);
private:
    struct TableInfo {
        BPlusTree<int, int> index;  // primary key -> record location
        BufferManager buffer;
    };
    std::unordered_map<std::string, TableInfo> tables_;
};
