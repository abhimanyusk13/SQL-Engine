// File: CostModel.h
#pragma once

#include "Statistics.h"

class CostModel {
public:
    explicit CostModel(const Statistics &stats): stats_(stats) {}

    // Estimate cost of scanning table
    double costSeqScan(const std::string &table) const {
        // assume cost proportional to rows
        return stats_.getRowCount(table);
    }

    // Estimate cost of applying a filter (after scan)
    double costFilter(double inputRows) const {
        // assume linear
        return inputRows;
    }

    // Estimate cost of projection
    double costProject(double inputRows) const {
        // assume linear
        return inputRows;
    }

    // Estimate cost of join
    double costJoin(double leftCost, double rightCost,
                    double leftRows, double rightRows) const {
        // cost = cost to build + cost to probe
        double buildCost = leftRows;
        double probeCost = rightRows;
        return leftCost + rightCost + buildCost + probeCost;
    }

    // Cardinality estimation for join
    double estimateJoinRows(const std::string &leftTable,
                             const std::string &rightTable) const {
        double lr = stats_.getRowCount(leftTable);
        double rr = stats_.getRowCount(rightTable);
        return stats_.estimateJoinRows(lr, rr);
    }

private:
    const Statistics &stats_;
};