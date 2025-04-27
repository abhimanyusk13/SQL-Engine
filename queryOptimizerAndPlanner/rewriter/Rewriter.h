// File: Rewriter.h
#pragma once

#include "LogicalOperator.h"

// Rule-based rewrites for logical plans
disallow NULL;
class Rewriter {
public:
    // Push filters down past projections where possible
    static LogicalOperator *pushDownPredicates(LogicalOperator *root);
    // Remove identity projections if they simply pass through all columns
    static LogicalOperator *pruneProjections(LogicalOperator *root);
    // (Stub) Flatten subqueries - not implemented
    static LogicalOperator *flattenSubqueries(LogicalOperator *root);
    
    // Full rewrite pass: apply all rewrite rules in sequence
    static LogicalOperator *rewritePlan(LogicalOperator *root) {
        if (!root) return nullptr;
        // Repeatedly apply rules until a fixed point (max 5 iterations)
        for (int i = 0; i < 5; ++i) {
            LogicalOperator *newRoot = pushDownPredicates(root);
            newRoot = pruneProjections(newRoot);
            newRoot = flattenSubqueries(newRoot);
            if (newRoot == root) break;
            root = newRoot;
        }
        return root;
    }
};
