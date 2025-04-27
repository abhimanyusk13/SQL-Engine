// File: PhysicalPlanGenerator.h
#pragma once

#include "LogicalOperator.h"
#include "StorageEngine.h"
#include "Catalog.h"
#include "PhysicalOperator.h"
#include "TableScan.h"
#include "Filter.h"
#include "Project.h"
#include "NestedLoopJoin.h"
#include <unordered_map>
#include <stdexcept>

class PhysicalPlanGenerator {
public:
    PhysicalPlanGenerator(StorageEngine &se, Catalog &catalog)
        : se_(se), catalog_(catalog) {}

    // Generate the root physical operator for a logical plan
    // Returns nullptr for non-SELECT statements
    physical::PhysicalOperator* generate(LogicalOperator *logical) {
        std::unordered_map<std::string,int> colIdx;
        return gen(logical, colIdx);
    }

private:
    StorageEngine &se_;
    Catalog &catalog_;

    // Recursive generator: populates colIdx for each subtree
    physical::PhysicalOperator* gen(LogicalOperator *node,
                                    std::unordered_map<std::string,int> &colIdx) {
        switch (node->opType) {
            case LogicalOpType::SeqScan: {
                auto *scan = static_cast<LogicalSeqScan*>(node);
                // Build scan
                auto *ts = new TableScan(se_, catalog_, scan->tableName);
                // Initialize column index map
                colIdx.clear();
                Schema schema = catalog_.getTable(scan->tableName);
                for (size_t i = 0; i < schema.numColumns(); ++i) {
                    std::string q = scan->tableName + "." + schema.getColumn(i).name;
                    colIdx[q] = static_cast<int>(i);
                }
                return ts;
            }
            case LogicalOpType::Filter: {
                auto *f = static_cast<LogicalFilter*>(node);
                // Child inherits current colIdx
                auto *childOp = gen(f->children[0], colIdx);
                return new Filter(childOp, f->predicate, colIdx);
            }
            case LogicalOpType::Project: {
                auto *p = static_cast<LogicalProject*>(node);
                // Generate child, get its colIdx
                auto *childOp = gen(p->children[0], colIdx);
                // Prepare output colIdx
                std::unordered_map<std::string,int> outIdx;
                for (size_t i = 0; i < p->projections.size(); ++i) {
                    // Use expression string or alias as new column name
                    std::string name = "col" + std::to_string(i);
                    outIdx[name] = static_cast<int>(i);
                }
                // Create Project operator
                auto *projOp = new Project(childOp, p->projections, colIdx);
                // Replace colIdx with output mapping
                colIdx = std::move(outIdx);
                return projOp;
            }
            case LogicalOpType::Join: {
                auto *j = static_cast<LogicalJoin*>(node);
                // Generate left subtree, capture its colIdx
                std::unordered_map<std::string,int> leftIdx;
                auto *leftOp = gen(j->children[0], leftIdx);
                // Generate right subtree, updating colIdx
                auto *rightOp = gen(j->children[1], colIdx);
                // Combine column maps: left followed by right
                std::unordered_map<std::string,int> combined;
                int pos = 0;
                for (auto &p : leftIdx) combined[p.first] = pos++;
                for (auto &p : colIdx)       combined[p.first] = pos++;
                colIdx = std::move(combined);
                return new NestedLoopJoin(leftOp, rightOp);
            }
            default:
                // INSERT, UPDATE, DELETE, CREATE, BEGIN, COMMIT not handled here
                return nullptr;
        }
    }
};