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

    std::vector<physical::Row> executeQuery(const std::string &sql) {
        AST ast = parser_.parse(sql);
        binder_.bind(ast);
        // Handle DML directly
        if (ast.stmtType == StmtType::INSERT) {
            auto *ins = ast.insert.get();
            vector<FieldValue> vals;
            for (auto &ePtr : ins->values) {
                Expr *e = ePtr.get();
                if (e->type == Expr::Type::INT_LITERAL)
                    vals.emplace_back(e->intValue);
                else if (e->type == Expr::Type::STR_LITERAL)
                    vals.emplace_back(e->strValue);
                else throw runtime_error("Unsupported INSERT expression");
            }
            se_.insertRecord(ins->table, vals);
            return {};
        }
        if (ast.stmtType == StmtType::UPDATE) {
            auto *upd = ast.update.get();
            // Only support PK equality in WHERE
            Expr *w = upd->whereClause.get();
            if (!(w->type==Expr::Type::BINARY_OP && w->op=="="))
                throw runtime_error("UPDATE only supports PK = value in WHERE");
            string pkCol = w->left->columnName;
            int32_t key = w->right->intValue;
            // Fetch existing record
            RecordID rid;
            se_.findByKey(upd->table, key, rid);
            auto oldVals = se_.fetchRecord(upd->table, rid);
            // Apply assignments
            for (auto &pr : upd->assignments) {
                const string &col = pr.first;
                Expr *e = pr.second.get();
                int idx = catalog_.getColumnInfo(upd->table, col).type==DataType::INT
                          ? (int)catalog_.getTable(upd->table).getColumnIndex(col)
                          : (int)catalog_.getTable(upd->table).getColumnIndex(col);
                if (e->type==Expr::Type::INT_LITERAL)
                    oldVals[idx] = e->intValue;
                else if (e->type==Expr::Type::STR_LITERAL)
                    oldVals[idx] = e->strValue;
                else throw runtime_error("Unsupported UPDATE expression");
            }
            se_.updateByKey(upd->table, key, oldVals);
            return {};
        }
        if (ast.stmtType == StmtType::DELETE) {
            auto *del = ast.deleteStmt.get();
            Expr *w = del->whereClause.get();
            if (!(w->type==Expr::Type::BINARY_OP && w->op=="="))
                throw runtime_error("DELETE only supports PK = value in WHERE");
            string pkCol = w->left->columnName;
            int32_t key = w->right->intValue;
            se_.deleteByKey(del->table, key);
            return {};
        }
        // Otherwise SELECT
        auto *logical = planner_.buildLogicalPlan(ast);
        logical = Rewriter::rewritePlan(logical);
        auto *physicalPlan = physGen_.generate(logical);
        auto rows = executor::Executor::execute(physicalPlan);
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