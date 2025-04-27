// File: Statistics.h
#pragma once

#include <string>
#include <unordered_map>

// Simple statistics for tables and join combinations
class Statistics {
public:
    // Register row count for a base table
    void setRowCount(const std::string &table, double rowCount) {
        baseRowCount_[table] = rowCount;
    }
    // Get row count for a base table
    double getRowCount(const std::string &table) const {
        return baseRowCount_.at(table);
    }
    // Estimate result rows for a join between two sets
    // using a simple uniform distribution assumption
    double estimateJoinRows(double leftRows, double rightRows) const {
        // naive cross-product scaled down
        return leftRows * rightRows * joinSelectivity_;  
    }
    void setJoinSelectivity(double s) { joinSelectivity_ = s; }
private:
    std::unordered_map<std::string, double> baseRowCount_;
    double joinSelectivity_ = 0.1; // default selectivity factor
};