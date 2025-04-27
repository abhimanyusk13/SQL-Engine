// File: QueryEngine.h
#pragma once

#include <string>
#include <vector>
#include "Parser.h"
#include "Binder.h"
#include "Planner.h"
#include "Rewriter.h"
#include "Statistics.h"
#include "CostModel.h"
#include "Optimizer.h"
#include "PhysicalPlanGenerator.h"

class QueryEngine {
public:
    QueryEngine(const std::string &catalogDir)
        : binder_(catalog_), planner_(), stats_(), costModel_(stats_), optimizer_(), fm_(), bm_(fm_), se_(fm_, bm_), physGen_(se_, catalog_) {
        catalog_.load(catalogDir);
    }

    // Execute a SQL SELECT query and return result rows
    std::vector<physical::Row> executeQuery(const std::string &sql) {
        // 1. Parse
        AST ast = parser_.parse(sql);
        // 2. Bind
        binder_.bind(ast);
        // 3. Build logical plan
        LogicalOperator *logical = planner_.buildLogicalPlan(ast);
        // 4. Rewrite
        logical = Rewriter::rewritePlan(logical);
        // 5. (Optional) Join optimization: not applied for single-table
        // 6. Generate physical plan
        auto *physicalPlan = physGen_.generate(logical);
        // 7. Execute
        auto rows = executor::Executor::execute(physicalPlan);
        // Clean up
        delete physicalPlan;
        deleteLogicalTree(logical);
        return rows;
    }

private:
    Parser parser_;
    Catalog catalog_;
    Binder binder_;
    Planner planner_;
    Statistics stats_;
    CostModel costModel_;
    Optimizer optimizer_;
    FileManager fm_;
    BufferManager bm_;
    StorageEngine se_;
    PhysicalPlanGenerator physGen_;
    
    // Recursively delete logical operator tree
    void deleteLogicalTree(LogicalOperator *node) {
        for (auto *c : node->children) deleteLogicalTree(c);
        delete node;
    }
};