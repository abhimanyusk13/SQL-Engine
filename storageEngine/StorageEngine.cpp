// File: StorageEngine.cpp
#include "StorageEngine.h"
#include <stdexcept>

StorageEngine::StorageEngine(FileManager &fm, BufferManager &bm)
    : fm_(fm), bm_(bm) {}

void StorageEngine::registerTable(const std::string &tableName,
                                  const Schema &schema,
                                  const std::string &dataFile,
                                  const std::string &indexFile,
                                  const std::string &primaryKeyColumn) {
    if (tables_.count(tableName))
        throw std::runtime_error("Table already registered: " + tableName);

    // Open or create underlying files
    fm_.openFile(dataFile);
    fm_.openFile(indexFile);

    // Construct heap and index
    auto heap = std::make_unique<TableHeap>(fm_, bm_, dataFile, schema);
    auto idx  = std::make_unique<BPlusTree<int32_t, RecordID>>(fm_.openFile(indexFile), bm_);

    // Locate PK column index
    int pkIdx = -1;
    for (size_t i = 0; i < schema.numColumns(); ++i) {
        if (schema.getColumn(i).name == primaryKeyColumn) {
            pkIdx = static_cast<int>(i);
            break;
        }
    }
    if (pkIdx < 0)
        throw std::runtime_error("Primary key column not found: " + primaryKeyColumn);

    tables_.emplace(tableName, TableInfo{schema, std::move(heap), std::move(idx), pkIdx});
}

RecordID StorageEngine::insertRecord(const std::string &tableName,
                                     const std::vector<FieldValue> &values) {
    auto it = tables_.find(tableName);
    if (it == tables_.end())
        throw std::runtime_error("Unknown table: " + tableName);
    TableInfo &ti = it->second;

    // Insert into heap
    RecordID rid = ti.heap->insertRecord(values);
    // Extract primary-key value (INT)
    FieldValue pkfv = values[ti.pkColIdx];
    if (!std::holds_alternative<int32_t>(pkfv))
        throw std::runtime_error("Primary key must be INT");
    int32_t key = std::get<int32_t>(pkfv);

    // Update B+ tree index
    ti.index->insert(key, rid);
    return rid;
}

bool StorageEngine::deleteByKey(const std::string &tableName, int32_t key) {
    auto it = tables_.find(tableName);
    if (it == tables_.end())
        throw std::runtime_error("Unknown table: " + tableName);
    TableInfo &ti = it->second;

    RecordID rid;
    if (!ti.index->find(key, rid))
        return false;

    bool ok = ti.heap->deleteRecord(rid);
    if (ok) ti.index->remove(key);
    return ok;
}

bool StorageEngine::updateByKey(const std::string &tableName,
                                int32_t key,
                                const std::vector<FieldValue> &values) {
    auto it = tables_.find(tableName);
    if (it == tables_.end())
        throw std::runtime_error("Unknown table: " + tableName);
    TableInfo &ti = it->second;

    RecordID rid;
    if (!ti.index->find(key, rid))
        return false;

    // Ensure PK unchanged
    FieldValue pkfv = values[ti.pkColIdx];
    if (!std::holds_alternative<int32_t>(pkfv) || std::get<int32_t>(pkfv) != key)
        throw std::runtime_error("Updating primary key is not supported");

    return ti.heap->updateRecord(rid, values);
}

bool StorageEngine::findByKey(const std::string &tableName,
                              int32_t key,
                              RecordID &outRid) const {
    auto it = tables_.find(tableName);
    if (it == tables_.end())
        throw std::runtime_error("Unknown table: " + tableName);
    const TableInfo &ti = it->second;
    return ti.index->find(key, outRid);
}

std::vector<RecordID> StorageEngine::scanTable(const std::string &tableName) const {
    auto it = tables_.find(tableName);
    if (it == tables_.end())
        throw std::runtime_error("Unknown table: " + tableName);
    const TableInfo &ti = it->second;
    return ti.heap->tableScan();
}

std::vector<FieldValue> StorageEngine::fetchRecord(const std::string &tableName,
    const RecordID &rid) const {
    auto it = tables_.find(tableName);
    if (it == tables_.end())
    throw std::runtime_error("Unknown table: " + tableName);
    const TableInfo &ti = it->second;
    Record rec = ti.heap->getRecord(rid);
    return rec.getValues();
}