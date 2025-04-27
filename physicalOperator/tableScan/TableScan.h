// File: TableScan.h
#pragma once
#include "PhysicalOperator.h"
#include "StorageEngine.h"
#include "Catalog.h"
#include <vector>
#include <string>
#include <unordered_map>

class TableScan : public physical::PhysicalOperator {
public:
    TableScan(StorageEngine &se, Catalog &catalog, const std::string &tableName);
    void open() override;
    bool next(physical::Row &row) override;
    void close() override;

private:
    StorageEngine &se_;
    Catalog &catalog_;
    std::string table_;
    std::vector<RecordID> rids_;
    size_t idx_;
    std::vector<std::string> colNames_;
    std::unordered_map<std::string,int> colIdx_;
};