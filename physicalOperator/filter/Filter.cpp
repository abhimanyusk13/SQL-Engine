// File: Filter.cpp
#include "Filter.h"

Filter::Filter(physical::PhysicalOperator *child,
               const Expr *predicate,
               std::unordered_map<std::string,int> colIdx)
    : child_(child), pred_(predicate), colIdx_(std::move(colIdx)) {}

void Filter::open() {
    child_->open();
}

bool Filter::next(physical::Row &row) {
    while (true) {
        if (!child_->next(row)) return false;
        if (ExpressionEvaluator::evalBoolean(pred_, row, colIdx_))
            return true;
    }
}

void Filter::close() {
    child_->close();
}