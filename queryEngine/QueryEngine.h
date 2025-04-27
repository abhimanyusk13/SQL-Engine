// File: QueryEngine.h
#pragma once

#include <string>
#include <vector>
#include "Parser.h"
#include "Binder.h"
#include "Planner.h"
#include "Rewriter.h"
#include "PhysicalPlanGenerator.h"
#include "Executor.h"
#include "Catalog.h"
#include "StorageEngine.h"
#include "TransactionManager.h"

class QueryEngine {
public:
    // Initialize the engine with a path to the catalog directory
    QueryEngine(const std::string &catalogPath);

    // Execute a SQL statement, returning result rows for SELECT
    std::vector<physical::Row> executeQuery(const std::string &sql);

private:
    // Transaction control
    void handleBegin();
    void handleCommit();
    void handleRollback();

    Parser parser_;
    Binder binder_;
    Planner planner_;
    Rewriter rewriter_;
    PhysicalPlanGenerator physGen_;
    executor::Executor executor_;
    Catalog catalog_;
    StorageEngine storage_;
    TransactionManager txMgr_;

    // Current transaction ID (0 if none)
    int64_t currentTxId_ = 0;
};