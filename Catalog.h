// File: Catalog.h
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "Schema.h"

// Simple index metadata
typedef struct IndexInfo {
    std::string name;
    std::vector<std::string> columns;
} IndexInfo;

class Catalog {
public:
    // Load metadata from the given directory (catalog.meta)
    void load(const std::string &dirPath);
    // Save metadata to the given directory (catalog.meta)
    void save(const std::string &dirPath) const;

    // Get table schema; throws if not found
    Schema getTable(const std::string &tableName) const;
    // Add a new table; initially no indexes and zero rows
    void addTable(const std::string &tableName, const Schema &schema);
    // Drop table metadata
    void dropTable(const std::string &tableName);

    // Get column info for a table
    Column getColumnInfo(const std::string &tableName,
                         const std::string &columnName) const;

    // Register a new index on the table
    void registerIndex(const std::string &tableName,
                       const std::string &indexName,
                       const std::vector<std::string> &columns);
    // Return list of indexes for a table
    std::vector<IndexInfo> getIndexes(const std::string &tableName) const;

    // Update table statistics (row count)
    void updateTableStats(const std::string &tableName, std::size_t rowCount);

private:
    struct TableMeta {
        Schema schema;
        std::vector<IndexInfo> indexes;
        std::size_t rowCount;
    };
    std::unordered_map<std::string, TableMeta> tables_;

    static std::string dataTypeToString(DataType dt);
    static DataType stringToDataType(const std::string &s);
};