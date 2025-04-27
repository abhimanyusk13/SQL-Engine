// File: ExpressionEvaluator.h
#pragma once
#include <unordered_map>
#include <string>
#include "AST.h"
#include "PhysicalOperator.h"

class ExpressionEvaluator {
public:
    // Evaluate arbitrary expression, returning FieldValue (INT or STRING)
    static FieldValue eval(const Expr *expr,
                           const physical::Row &row,
                           const std::unordered_map<std::string,int> &colIdx);
    // Evaluate predicate as boolean
    static bool evalBoolean(const Expr *expr,
                            const physical::Row &row,
                            const std::unordered_map<std::string,int> &colIdx);
};