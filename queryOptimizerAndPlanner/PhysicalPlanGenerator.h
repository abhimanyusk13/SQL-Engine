// File: PhysicalPlanGenerator.h
#pragma once

#include "LogicalOperator.h"
#include "StorageEngine.h"
#include "Catalog.h"
#include "PhysicalOperator.h"
#include "TableScan.h"
#include "Filter.h"
#include "Project.h"
#include <unordered_map>

class PhysicalPlanGenerator {
public:
    PhysicalPlanGenerator(StorageEngine &se, Catalog &catalog)
        : se_(se), catalog_(catalog) {}

    // Generate a physical operator tree from a logical plan
    physical::PhysicalOperator* generate(LogicalOperator *logical) {
        // Context: map columnName -> position in current row
        std::unordered_map<std::string,int> colIdx;
        return gen(logical, colIdx);
    }

private:
    StorageEngine &se_;
    Catalog &catalog_;

    physical::PhysicalOperator* gen(LogicalOperator *node,
                                    std::unordered_map<std::string,int> &colIdx) {
        switch (node->opType) {
        case LogicalOpType::SeqScan: {
            auto *scan = static_cast<LogicalSeqScan*>(node);
            auto *ts = new TableScan(se_, catalog_, scan->tableName);
            // Build column index map from schema
            Schema sch = catalog_.getTable(scan->tableName);
            colIdx.clear();
            for (size_t i = 0; i < sch.numColumns(); ++i) {
                std::string q = scan->tableName + "." + sch.getColumn(i).name;
                colIdx[q] = (int)i;
            }
            return ts;
        }
        case LogicalOpType::Filter: {
            auto *f = static_cast<LogicalFilter*>(node);
            // Generate child first
            auto *childOp = gen(f->children[0], colIdx);
            // Filter uses same colIdx
            return new Filter(childOp, f->predicate, colIdx);
        }
        case LogicalOpType::Project: {
            auto *p = static_cast<LogicalProject*>(node);
            // Generate child first
            auto *childOp = gen(p->children[0], colIdx);
            // New colIdx for projection output
            std::unordered_map<std::string,int> outIdx;
            for (size_t i = 0; i < p->projections.size(); ++i) {
                // Derive alias or repr as expr->toString()
                std::string name = "col" + std::to_string(i);
                outIdx[name] = (int)i;
            }
            // Project uses old colIdx to eval, but after projection, replace colIdx
            auto *projOp = new Project(childOp, p->projections, colIdx);
            colIdx = std::move(outIdx);
            return projOp;
        }
        case LogicalOpType::Join: {
            auto *j = static_cast<LogicalJoin*>(logical);
            // Generate left and right
            auto *leftOp  = gen(j->children[0], colIdx);
            // Capture left schema columns
            auto leftColIdx = colIdx;
            auto leftCols = colIdx;
            auto *rightOp = gen(j->children[1], colIdx);
            // Build combined colIdx
            std::unordered_map<std::string,int> combined;
            int pos = 0;
            for (auto &p : leftColIdx) combined[p.first] = pos++;
            for (auto &p : colIdx) combined[p.first] = pos++;
            colIdx = combined;
            return new NestedLoopJoin(leftOp, rightOp);
        }
        default:
            throw std::runtime_error("PhysicalPlanGenerator: unsupported logical op type");
        }
    }
};