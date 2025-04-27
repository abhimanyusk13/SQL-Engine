// File: Project.h
#pragma once
#include "PhysicalOperator.h"
#include "AST.h"
#include "ExpressionEvaluator.h"
#include <vector>
#include <unordered_map>

class Project : public physical::PhysicalOperator {
public:
    Project(physical::PhysicalOperator *child,
            std::vector<Expr*> exprs,
            std::unordered_map<std::string,int> colIdx);
    void open() override;
    bool next(physical::Row &outRow) override;
    void close() override;

private:
    physical::PhysicalOperator *child_;
    std::vector<Expr*> exprs_;
    std::unordered_map<std::string,int> colIdx_;
};