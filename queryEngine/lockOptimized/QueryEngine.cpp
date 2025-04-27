// File: QueryEngine.cpp
#include "QueryEngine.h"
#include "MetricsManager.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

static std::string trim(const std::string &s) {
    auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e-b+1);
}

static bool iequals_prefix(const std::string &s, const char *p) {
    size_t n = strlen(p);
    if (s.size() < n) return false;
    for (size_t i = 0; i < n; i++) {
        if (std::toupper(s[i]) != p[i]) return false;
    }
    return true;
}

QueryEngine::QueryEngine(const std::string &catalogDir)
  : fm_()
  , bm_(fm_, 128)
  , catalog_(catalogDir)
  , lockMgr_()
  , walMgr_(catalogDir + "/wal.log")
  , storage_(fm_, bm_, catalog_, lockMgr_, walMgr_)
  , txMgr_()
  , statsMgr_(storage_, catalog_)
  , parser_()
  , binder_(catalog_)
  , planner_()
  , rewriter_()
  , optimizer_()
  , physGen_()
  , executor_()
  , currentTxId_(0)
{
    // 1) Recover from WAL before serving anything
    walMgr_.recover(storage_);
}

void QueryEngine::registerTable(const std::string &name,
                                const Schema &schema,
                                const std::string &df,
                                const std::string &ifile,
                                const std::string &pk)
{
    storage_.registerTable(name, schema, df, ifile, pk);
}

std::vector<physical::Row>
QueryEngine::executeQuery(const std::string &rawSql)
{
    auto sql = trim(rawSql);

    // 1) ANALYZE tableName
    if (iequals_prefix(sql, "ANALYZE")) {
        // strip "ANALYZE"
        std::string rest = trim(sql.substr(7));
        // optionally strip "TABLE"
        if (iequals_prefix(rest, "TABLE"))
            rest = trim(rest.substr(5));
        if (rest.empty())
            throw std::runtime_error("ANALYZE requires a table name");
        statsMgr_.analyzeTable(rest);
        return {};
    }

    // 2) Transaction control
    if (iequals_prefix(sql, "BEGIN")) {
        if (currentTxId_ != 0)
            throw std::runtime_error("Nested transactions not supported");
        currentTxId_ = txMgr_.beginTransaction();
        return {};
    }
    if (iequals_prefix(sql, "COMMIT")) {
        if (currentTxId_ == 0)
            throw std::runtime_error("No active TX to commit");
        walMgr_.logCommit(currentTxId_);
        walMgr_.flush();
        txMgr_.commitTransaction(currentTxId_);
        lockMgr_.releaseTransactionLocks(currentTxId_);
        MetricsManager::instance().incTransactionsCommitted();
        currentTxId_ = 0;
        return {};
    }
    if (iequals_prefix(sql, "ROLLBACK")) {
        if (currentTxId_ == 0)
            throw std::runtime_error("No active TX to rollback");
        walMgr_.logAbort(currentTxId_);
        walMgr_.flush();
        txMgr_.abortTransaction(currentTxId_);
        lockMgr_.releaseTransactionLocks(currentTxId_);
        MetricsManager::instance().incTransactionsAborted();
        currentTxId_ = 0;
        return {};
    }

    // 3) Parse & bind
    AST ast = parser_.parse(sql);
    binder_.bind(ast);

    // 4) DML (inside transaction)
    if (ast.isInsert() || ast.isUpdate() || ast.isDelete()) {
        if (currentTxId_ == 0)
            throw std::runtime_error("DML must be inside a transaction");
        if (ast.isInsert()) {
            auto &ins = ast.getInsert();
            storage_.insertRecord(ins.tableName,
                                  ins.columnNames,
                                  ins.values,
                                  currentTxId_);
        } else if (ast.isUpdate()) {
            auto &up = ast.getUpdate();
            storage_.updateRecords(up.tableName,
                                   up.assignments,
                                   up.whereClause,
                                   currentTxId_);
        } else {
            auto &del = ast.getDelete();
            storage_.deleteRecords(del.tableName,
                                   del.whereClause,
                                   currentTxId_);
        }
        return {};
    }

    // 5) SELECT
    if (ast.isSelect()) {
        auto t0 = std::chrono::steady_clock::now();
        auto lplan = planner_.buildLogicalPlan(ast);
        auto wrp   = rewriter_.rewritePlan(lplan);
        auto opt   = optimizer_.optimize(wrp);
        auto prow  = physGen_.generate(opt);
        auto rows  = executor_.execute(prow);
        auto t1 = std::chrono::steady_clock::now();
        uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        auto &mm = MetricsManager::instance();
        mm.incQueryCount();
        mm.recordQueryLatencyUs(us);
        return rows;
    }

    // Other DDL (CREATE TABLE, etc.) could go here
    return {};
}
