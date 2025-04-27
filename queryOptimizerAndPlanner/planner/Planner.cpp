/ File: Planner.cpp
#include "Planner.h"
#include <stdexcept>

LogicalOperator* Planner::buildLogicalPlan(const AST &ast) {
    switch (ast.stmtType) {
        case StmtType::SELECT: {
            auto *sel = ast.select.get();
            // Only single-table supported for now
            if (sel->tables.size() != 1)
                throw std::runtime_error("Only single-table SELECT is supported");
            // 1) SeqScan
            LogicalOperator *plan = new LogicalSeqScan(sel->tables[0]);
            // 2) Filter (if any)
            if (sel->whereClause) {
                plan = new LogicalFilter(sel->whereClause.get(), plan);
            }
            // 3) Project
            std::vector<Expr*> projExprs;
            for (auto &ePtr : sel->selectList) projExprs.push_back(ePtr.get());
            plan = new LogicalProject(projExprs, plan);
            return plan;
        }
        case StmtType::INSERT: {
            auto *ins = ast.insert.get();
            // Columns list and values
            std::vector<Expr*> vals;
            for (auto &vPtr : ins->values) vals.push_back(vPtr.get());
            return new LogicalInsert(ins->table, ins->columns, vals);
        }
        case StmtType::UPDATE: {
            auto *upd = ast.update.get();
            std::vector<std::pair<std::string,Expr*>> assigns;
            for (auto &p : upd->assignments) assigns.emplace_back(p.first, p.second.get());
            return new LogicalUpdate(upd->table, assigns, upd->whereClause.get());
        }
        case StmtType::DELETE: {
            auto *del = ast.deleteStmt.get();
            return new LogicalDelete(del->table, del->whereClause.get());
        }
        case StmtType::CREATE_TABLE: {
            auto *ct = ast.createTable.get();
            return new LogicalCreateTable(ct->table, ct->columns);
        }
        case StmtType::BEGIN:
            return new LogicalBegin();
        case StmtType::COMMIT:
            return new LogicalCommit();
        default:
            throw std::runtime_error("Unsupported statement in planner");
    }
}
