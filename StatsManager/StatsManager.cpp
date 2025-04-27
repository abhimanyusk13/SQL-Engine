// File: StatsManager.cpp
#include "StatsManager.h"
#include <unordered_map>
#include <unordered_set>
#include <string>

// Helper: stringify a FieldValue
static std::string fvToString(const FieldValue &fv) {
    if (std::holds_alternative<int32_t>(fv))
        return std::to_string(std::get<int32_t>(fv));
    else
        return std::get<std::string>(fv);
}

StatsManager::StatsManager(StorageEngine &storage, Catalog &catalog)
  : storage_(storage), catalog_(catalog) {}

void StatsManager::analyzeTable(const std::string &tableName) {
    // 1) Fetch schema & all RIDs
    Schema schema = catalog_.getTableSchema(tableName);
    auto rids = storage_.scanTable(tableName);

    // 2) Count rows
    int64_t rowCount = static_cast<int64_t>(rids.size());

    // 3) Prepare per‐column distinct counters
    std::unordered_map<std::string, std::unordered_set<std::string>> distinctVals;
    for (int i = 0; i < schema.numColumns(); ++i)
        distinctVals[schema.getColumn(i).name] = {};

    // 4) Scan each record
    for (auto &rid : rids) {
        auto row = storage_.getRecord(tableName, rid);
        for (int i = 0; i < schema.numColumns(); ++i) {
            const auto &colName = schema.getColumn(i).name;
            distinctVals[colName].insert(fvToString(row[i]));
        }
    }

    // 5) Build a simple map of distinct‐value counts
    std::unordered_map<std::string,int64_t> distinctCounts;
    for (auto &p : distinctVals) {
        distinctCounts[p.first] = static_cast<int64_t>(p.second.size());
    }

    // 6) Push into the catalog
    catalog_.updateTableStats(tableName, rowCount, distinctCounts);
}
