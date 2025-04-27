// File: Catalog.cpp
#include "Catalog.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>

void Catalog::load(const std::string &dirPath) {
    std::string path = dirPath + "/catalog.meta";
    if (!std::filesystem::exists(path)) return;
    std::ifstream in(path);
    if (!in.is_open()) return;
    tables_.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("TABLE ", 0) == 0) {
            std::string tableName = line.substr(6);
            // Read COLUMNS
            std::getline(in, line);
            std::istringstream cinCols(line);
            std::string tag;
            size_t numCols;
            cinCols >> tag >> numCols;
            std::vector<Column> cols;
            for (size_t i = 0; i < numCols; ++i) {
                std::getline(in, line);
                std::istringstream colLine(line);
                std::string colName, dt;
                size_t length;
                colLine >> colName >> dt >> length;
                cols.push_back(Column{colName, stringToDataType(dt), length});
            }
            Schema schema(cols);
            // Read INDEXES
            std::getline(in, line);
            std::istringstream cinIdx(line);
            size_t numIdx;
            cinIdx >> tag >> numIdx;
            std::vector<IndexInfo> idxs;
            for (size_t i = 0; i < numIdx; ++i) {
                std::getline(in, line);
                std::istringstream idxLine(line);
                std::string idxName;
                size_t cnt;
                idxLine >> idxName >> cnt;
                std::vector<std::string> colsList;
                for (size_t j = 0; j < cnt; ++j) {
                    std::string nm;
                    idxLine >> nm;
                    colsList.push_back(nm);
                }
                idxs.push_back(IndexInfo{idxName, colsList});
            }
            // Read STATS
            std::getline(in, line);
            std::istringstream cinStats(line);
            std::size_t rowCount;
            cinStats >> tag >> rowCount;
            // Expect END
            std::getline(in, line); // "END"
            tables_[tableName] = TableMeta{schema, idxs, rowCount};
        }
    }
    in.close();
}

void Catalog::save(const std::string &dirPath) const {
    std::filesystem::create_directories(dirPath);
    std::string path = dirPath + "/catalog.meta";
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open())
        throw std::runtime_error("Unable to write catalog: " + path);
    for (auto & [tableName, meta] : tables_) {
        out << "TABLE " << tableName << '\n';
        // COLUMNS
        out << "COLUMNS " << meta.schema.numColumns() << '\n';
        for (size_t i = 0; i < meta.schema.numColumns(); ++i) {
            const Column &c = meta.schema.getColumn(i);
            out << c.name << ' ' << dataTypeToString(c.type) << ' ' << c.length << '\n';
        }
        // INDEXES
        out << "INDEXES " << meta.indexes.size() << '\n';
        for (auto &idx : meta.indexes) {
            out << idx.name << ' ' << idx.columns.size();
            for (auto &col : idx.columns) out << ' ' << col;
            out << '\n';
        }
        // STATS
        out << "STATS " << meta.rowCount << '\n';
        out << "END" << '\n';
    }
    out.close();
}

Schema Catalog::getTable(const std::string &tableName) const {
    auto it = tables_.find(tableName);
    if (it == tables_.end())
        throw std::runtime_error("Table not found: " + tableName);
    return it->second.schema;
}

void Catalog::addTable(const std::string &tableName, const Schema &schema) {
    if (tables_.count(tableName))
        throw std::runtime_error("Table already exists: " + tableName);
    tables_[tableName] = TableMeta{schema, {}, 0};
}

void Catalog::dropTable(const std::string &tableName) {
    if (!tables_.erase(tableName))
        throw std::runtime_error("Table not found: " + tableName);
}

Column Catalog::getColumnInfo(const std::string &tableName,
                              const std::string &columnName) const {
    Schema schema = getTable(tableName);
    for (size_t i = 0; i < schema.numColumns(); ++i) {
        const Column &c = schema.getColumn(i);
        if (c.name == columnName) return c;
    }
    throw std::runtime_error("Column not found: " + columnName);
}

void Catalog::registerIndex(const std::string &tableName,
                            const std::string &indexName,
                            const std::vector<std::string> &columns) {
    auto it = tables_.find(tableName);
    if (it == tables_.end())
        throw std::runtime_error("Table not found: " + tableName);
    it->second.indexes.push_back(IndexInfo{indexName, columns});
}

std::vector<IndexInfo> Catalog::getIndexes(const std::string &tableName) const {
    auto it = tables_.find(tableName);
    if (it == tables_.end())
        throw std::runtime_error("Table not found: " + tableName);
    return it->second.indexes;
}

void Catalog::updateTableStats(const std::string &tableName, std::size_t rowCount) {
    auto it = tables_.find(tableName);
    if (it == tables_.end())
        throw std::runtime_error("Table not found: " + tableName);
    it->second.rowCount = rowCount;
}

std::string Catalog::dataTypeToString(DataType dt) {
    switch (dt) {
        case DataType::INT:    return "INT";
        case DataType::STRING: return "STRING";
    }
    return "UNKNOWN";
}

DataType Catalog::stringToDataType(const std::string &s) {
    if (s == "INT") return DataType::INT;
    if (s == "STRING") return DataType::STRING;
    throw std::runtime_error("Unknown DataType: " + s);
}