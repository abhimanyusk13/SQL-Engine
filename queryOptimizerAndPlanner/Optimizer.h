// File: Optimizer.h
#pragma once

#include "LogicalOperator.h"
#include "CostModel.h"
#include <vector>
#include <limits>
#include <unordered_map>

class Optimizer {
public:
    // Optimize a bushy join tree for two tables (extension needed for more tables)
    // Here we implement pairwise join order optimization using dynamic programming
    LogicalOperator* optimizeJoins(const std::vector<std::string> &tables,
                                   const CostModel &costModel,
                                   const std::unordered_map<std::string, LogicalOperator*> &scans) {
        int n = tables.size();
        // DP table: best cost for subset represented by bitmask
        int maxMask = 1 << n;
        struct PlanEntry { double cost; double rows; LogicalOperator* plan; };
        std::vector<PlanEntry> dp(maxMask, {std::numeric_limits<double>::infinity(), 0.0, nullptr});

        // Initialize single-table plans
        for (int i = 0; i < n; ++i) {
            int m = 1 << i;
            dp[m].cost = costModel.costSeqScan(tables[i]);
            dp[m].rows = costModel.stats_.getRowCount(tables[i]);
            dp[m].plan = scans.at(tables[i]);
        }

        // Build plans for larger subsets
        for (int mask = 1; mask < maxMask; ++mask) {
            // skip singletons
            if ((mask & (mask - 1)) == 0) continue;
            // partition mask into two non-empty subsets
            for (int left = (mask - 1) & mask; left; left = (left - 1) & mask) {
                int right = mask ^ left;
                auto &L = dp[left];
                auto &R = dp[right];
                if (!L.plan || !R.plan) continue;
                // estimate rows and cost
                double estRows = costModel.estimateJoinRows(tables[__builtin_ctz(left)],
                                                           tables[__builtin_ctz(right)]);
                double c = costModel.costJoin(L.cost, R.cost, L.rows, R.rows);
                if (c < dp[mask].cost) {
                    // Create a LogicalFilter or Join node (not defined)
                    // Here we wrap as Filter for demonstration
                    dp[mask].cost = c;
                    dp[mask].rows = estRows;
                    dp[mask].plan = new LogicalFilter(/* predicate */ nullptr, L.plan);
                    dp[mask].plan->children.push_back(R.plan);
                }
            }
        }
        return dp[maxMask - 1].plan;
    }
};
