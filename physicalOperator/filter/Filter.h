// File: Filter.h
#pragma once
#include "PhysicalOperator.h"
#include "AST.h"
#include "ExpressionEvaluator.h"
#include <unordered_map>

class Filter : public physical::PhysicalOperator {
public:
    Filter(physical::PhysicalOperator *child,
           const Expr *predicate,
           std::unordered_map<std::string,int> colIdx);
    void open() override;
    bool next(physical::Row &row) override;
    void close() override;

private:
    physical::PhysicalOperator *child_;
    const Expr *pred_;
    std::unordered_map<std::string,int> colIdx_;
};
