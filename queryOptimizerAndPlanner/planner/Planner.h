// File: Planner.h
#pragma once

#include "AST.h"
#include "LogicalOperator.h"

class Planner {
public:
    // Build a logical plan from a bound AST
    LogicalOperator* buildLogicalPlan(const AST &ast);
};