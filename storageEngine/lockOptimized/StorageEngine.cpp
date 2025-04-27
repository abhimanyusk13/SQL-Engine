#include "StorageEngine.h"
#include <stdexcept>

StorageEngine::StorageEngine(FileManager &fm,
                             BufferManager &bm,
                             Catalog &catalog,
                             LockManager &lockMgr,
                             WALManager &walMgr)
  : fm_(fm)
  , bm_(bm)
  , catalog_(catalog)
  , lockMgr_(lockMgr)
  , walMgr_(walMgr)
{}

void StorageEngine::registerTable(const std::string &name,
                                  const Schema &schema,
                                  const std::string &dataFile,
                                  const std::string &indexFile,
                                  const std::string &pkColumn) {
    catalog_.addTable(name, schema);
    int pkIdx = schema.columnIndex(pkColumn);
    if (pkIdx < 0) throw std::runtime_error("Unknown PK column: " + pkColumn);

    auto heap = std::make_unique<TableHeap>(fm_, bm_, dataFile, schema);
    auto idx  = std::make_unique<PKIndex>(fm_, bm_, indexFile, /*order=*/128);

    tables_.emplace(
      name,
      TableData{std::move(heap), std::move(idx), pkIdx});
}

RecordID StorageEngine::insertRecord(const std::string &tableName,
                                     const std::vector<std::string> &cols,
                                     const std::vector<FieldValue> &vals,
                                     int64_t txId)
{
    auto &td = tables_.at(tableName);
    auto &sch = catalog_.getTableSchema(tableName);

    // 1) lock
    lockMgr_.lockExclusive(txId, "table:" + tableName);

    // 2) build full field vector
    std::vector<FieldValue> fv(sch.numColumns());
    for (size_t i = 0; i < cols.size(); i++) {
        int idx = sch.columnIndex(cols[i]);
        if (idx < 0) throw std::runtime_error("Unknown column " + cols[i]);
        fv[idx] = vals[i];
    }
    for (int i = 0; i < sch.numColumns(); i++) {
        if (!fv[i].has_value())
            fv[i] = sch.defaultFor(i);
    }

    // 3) do the insert
    RecordID rid = td.heap->insertRecord(fv);

    // 4) log it
    walMgr_.logInsert(txId, tableName, rid, fv);

    // 5) index
    auto &pkv = std::get<int32_t>(fv[td.pkColIdx]);
    td.index->insert(pkv, rid);

    return rid;
}

void StorageEngine::deleteRecords(const std::string &tableName,
                                  Expr *where,
                                  int64_t txId)
{
    auto &td = tables_.at(tableName);
    lockMgr_.lockExclusive(txId, "table:" + tableName);

    // assume WHERE pk = literal
    // ... resolve key as before ...
    int32_t key = /*extract from where*/ std::get<int32_t>(where->right->intValue);

    auto optRid = td.index->search(key);
    if (!optRid) return;

    // fetch old
    auto oldVals = td.heap->getRecord(*optRid);

    // log delete
    walMgr_.logDelete(txId, tableName, *optRid, oldVals);

    // delete
    td.heap->deleteRecord(*optRid);
    td.index->remove(key);
}

void StorageEngine::updateRecords(const std::string &tableName,
                                  const std::vector<std::pair<std::string, Expr*>> &assigns,
                                  Expr *where,
                                  int64_t txId)
{
    auto &td = tables_.at(tableName);
    lockMgr_.lockExclusive(txId, "table:" + tableName);

    // assume WHERE pk = literal; extract key...
    int32_t key = /*...*/0;
    auto optRid = td.index->search(key);
    if (!optRid) return;

    // old/new
    auto oldVals = td.heap->getRecord(*optRid);
    auto newVals = oldVals;
    for (auto &pr : assigns) {
        int idx = catalog_.getTableSchema(tableName).columnIndex(pr.first);
        newVals[idx] = ExpressionEvaluator::eval(pr.second, oldVals,
                         catalog_.getTableSchema(tableName));
    }

    // log update
    walMgr_.logUpdate(txId, tableName, *optRid, oldVals, newVals);

    // apply
    td.heap->updateRecord(*optRid, newVals);
}

std::vector<FieldValue>
StorageEngine::getRecord(const std::string &tableName,
                         const RecordID &rid)
{
    auto &td = tables_.at(tableName);
    return td.heap->getRecord(rid);
}

std::vector<RecordID>
StorageEngine::scanTable(const std::string &tableName,
                         int64_t txId)
{
    auto &td = tables_.at(tableName);
    // shared lock for the whole table
    lockMgr_.lockShared(txId, "table:" + tableName);
    return td.heap->tableScan();
}
