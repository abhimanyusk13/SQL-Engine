#include "QueryEngine.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

static std::string trim(const std::string &s) {
    auto b = s.find_first_not_of(" \t\r\n");
    if (b==std::string::npos) return "";
    auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e-b+1);
}

static bool iequals_prefix(const std::string &s, const char *p) {
    size_t n = strlen(p);
    if (s.size()<n) return false;
    for (size_t i=0;i<n;i++)
        if (std::toupper(s[i])!=p[i]) return false;
    return true;
}

QueryEngine::QueryEngine(const std::string &catalogDir)
  : fm_()
  , bm_(fm_, /*pool*/128)
  , catalog_(catalogDir)
  , lockMgr_()
  , walMgr_(catalogDir + "/wal.log")
  , storage_(fm_, bm_, catalog_, lockMgr_, walMgr_)
  , txMgr_()
  , parser_()
  , binder_()
  , planner_()
  , rewriter_()
  , optimizer_()
  , physGen_()
  , executor_()
  , currentTxId_(0)
{
    // 1) recover
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

    // TRANSACTION CONTROL
    if (iequals_prefix(sql,"BEGIN")) {
        if (currentTxId_!=0)
            throw std::runtime_error("Nested TX not supported");
        currentTxId_ = txMgr_.beginTransaction();
        return {};
    }
    if (iequals_prefix(sql,"COMMIT")) {
        if (currentTxId_==0)
            throw std::runtime_error("No active TX to commit");
        walMgr_.logCommit(currentTxId_);
        walMgr_.flush();
        txMgr_.commitTransaction(currentTxId_);
        lockMgr_.releaseTransactionLocks(currentTxId_);
        currentTxId_ = 0;
        return {};
    }
    if (iequals_prefix(sql,"ROLLBACK")) {
        if (currentTxId_==0)
            throw std::runtime_error("No active TX to rollback");
        walMgr_.logAbort(currentTxId_);
        walMgr_.flush();
        txMgr_.abortTransaction(currentTxId_);
        lockMgr_.releaseTransactionLocks(currentTxId_);
        currentTxId_ = 0;
        return {};
    }

    // PARSE + BIND
    AST ast = parser_.parse(sql);
    binder_.bind(ast);

    // DML (must be in TX)
    if (ast.isInsert() || ast.isUpdate() || ast.isDelete()) {
        if (currentTxId_==0)
            throw std::runtime_error("DML must be inside a transaction");
        if (ast.isInsert()) {
            auto &ins = ast.getInsert();
            storage_.insertRecord(ins.tableName, ins.columnNames, ins.values, currentTxId_);
        }
        else if (ast.isUpdate()) {
            auto &up = ast.getUpdate();
            storage_.updateRecords(up.tableName, up.assignments, up.whereClause, currentTxId_);
        }
        else {
            auto &del = ast.getDelete();
            storage_.deleteRecords(del.tableName, del.whereClause, currentTxId_);
        }
        return {};
    }

    // SELECT
    if (ast.isSelect()) {
        auto lp    = planner_.buildLogicalPlan(ast);
        auto wrp   = rewriter_.rewritePlan(lp);
        auto opt   = optimizer_.optimize(wrp);
        auto prow  = physGen_.generate(opt);
        return executor_.execute(prow);
    }

    // other DDL...
    return {};
}
