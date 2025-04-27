// File: Rewriter.cpp
#include "Rewriter.h"
#include <stdexcept>

// Recursively apply push-down predicate rule
LogicalOperator *Rewriter::pushDownPredicates(LogicalOperator *root) {
    if (!root) return nullptr;
    // First rewrite children
    for (auto &child : root->children) {
        child = pushDownPredicates(child);
    }
    // If we have Filter over Project, swap them
    if (root->opType == LogicalOpType::Filter) {
        auto *filter = static_cast<LogicalFilter*>(root);
        auto *child = filter->children[0];
        if (child->opType == LogicalOpType::Project) {
            auto *proj = static_cast<LogicalProject*>(child);
            // detach filter from project
            filter->children[0] = proj->children[0];
            // reattach
            proj->children[0] = filter;
            return proj;  // new root of this subtree
        }
    }
    return root;
}

LogicalOperator *Rewriter::pruneProjections(LogicalOperator *root) {
    if (!root) return nullptr;
    // First rewrite children
    for (auto &child : root->children) {
        child = pruneProjections(child);
    }
    // Remove Project if it directly wraps a SeqScan and passes through all columns
    if (root->opType == LogicalOpType::Project) {
        auto *proj = static_cast<LogicalProject*>(root);
        auto *child = proj->children[0];
        if (child->opType == LogicalOpType::SeqScan) {
            auto *scan = static_cast<LogicalSeqScan*>(child);
            // If projection list is exactly the table's columns in order (col0, col1, ...)
            bool identity = true;
            for (size_t i = 0; i < proj->projections.size(); ++i) {
                Expr *e = proj->projections[i];
                if (e->type != Expr::Type::COLUMN_REF ||
                    e->columnName != scan->tableName + "." + std::to_string(i)) {
                    identity = false;
                    break;
                }
            }
            if (identity) {
                return child;  // drop this Project
            }
        }
    }
    return root;
}

LogicalOperator *Rewriter::flattenSubqueries(LogicalOperator *root) {
    // Subquery flattening not implemented
    return root;
}