// File: TableScan.cpp
#include "TableScan.h"
#include <stdexcept>

TableScan::TableScan(StorageEngine &se, Catalog &catalog, const std::string &tableName)
    : se_(se), catalog_(catalog), table_(tableName), idx_(0) {
    // Build qualified column names
    Schema schema = catalog_.getTable(tableName);
    for (size_t i = 0; i < schema.numColumns(); ++i) {
        auto col = schema.getColumn(i).name;
        std::string q = table_ + "." + col;
        colIdx_[q] = (int)i;
        colNames_.push_back(q);
    }
}

void TableScan::open() {
    rids_ = se_.scanTable(table_);
    idx_ = 0;
}

bool TableScan::next(physical::Row &row) {
    while (idx_ < rids_.size()) {
        RecordID rid = rids_[idx_++];
        row = se_.fetchRecord(table_, rid);
        return true;
    }
    return false;
}

void TableScan::close() {
    rids_.clear();
    idx_ = 0;
}
