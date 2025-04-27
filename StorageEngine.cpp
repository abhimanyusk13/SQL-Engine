// File: StorageEngine.cpp
#include "StorageEngine.h"

void StorageEngine::openTable(const std::string &name) {
    // TODO: load or create data/index files
}

void StorageEngine::insertRecord(const std::string &table, const Record &rec) {
    // TODO: write to buffer, update B+ index
}

std::vector<Record> StorageEngine::scanTable(const std::string &table) {
    // TODO: full table scan via buffer manager or rangeScan on index
    return {};
}
