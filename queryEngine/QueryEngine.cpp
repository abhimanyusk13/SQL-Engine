// File: QueryEngine.cpp
#include "QueryEngine.h"
#include <stdexcept>

QueryEngine::QueryEngine(const std::string &catalogPath)
    : parser_(), binder_(), planner_(), rewriter_(), physGen_(), executor_(),
      catalog_(), storage_(catalog_), txMgr_() {
    catalog_.load(catalogPath);
    storage_.initialize(catalog_);
}

std::vector<physical::Row> QueryEngine::executeQuery(const std::string &sql) {
    // 1. Parse SQL text
    auto ast = parser_.parse(sql);

    // 2. Handle transaction control statements
    switch (ast->type) {
    case AST::StmtType::BEGIN:
        handleBegin();
        return {};
    case AST::StmtType::COMMIT:
        handleCommit();
        return {};
    case AST::StmtType::ROLLBACK:
        handleRollback();
        return {};
    default:
        break;
    }

    // 3. Semantic binding
    binder_.bind(ast.get(), catalog_);

    // 4. DML outside of SELECT must be in a transaction
    if (ast->type == AST::StmtType::INSERT ||
        ast->type == AST::StmtType::UPDATE ||
        ast->type == AST::StmtType::DELETE) {
        if (currentTxId_ == 0) {
            throw std::runtime_error("No active transaction. Please BEGIN first.");
        }
    }

    // 5. Build logical plan
    auto logicalPlan = planner_.buildLogicalPlan(ast.get());

    // 6. Rewrite logical plan
    logicalPlan = rewriter_.rewritePlan(std::move(logicalPlan));

    // 7. Cost-based optimization (joins, etc.)
    logicalPlan = planner_.optimizePlan(std::move(logicalPlan), catalog_);

    // 8. Physical plan generation
    auto physicalPlan = physGen_.generate(std::move(logicalPlan), catalog_, storage_);

    // 9. Execute plan
    auto rows = executor_.execute(physicalPlan.get());

    // 10. Return rows (empty for DML)
    return rows;
}

void QueryEngine::handleBegin() {
    if (currentTxId_ != 0) {
        throw std::runtime_error("Transaction already in progress");
    }
    currentTxId_ = txMgr_.beginTransaction();
}

void QueryEngine::handleCommit() {
    if (currentTxId_ == 0) {
        throw std::runtime_error("No active transaction to commit");
    }
    txMgr_.commitTransaction(currentTxId_);
    currentTxId_ = 0;
}

void QueryEngine::handleRollback() {
    if (currentTxId_ == 0) {
        throw std::runtime_error("No active transaction to rollback");
    }
    txMgr_.abortTransaction(currentTxId_);
    currentTxId_ = 0;
}
