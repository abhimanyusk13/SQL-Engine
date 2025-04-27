// File: Project.cpp
#include "Project.h"

Project::Project(physical::PhysicalOperator *child,
                 std::vector<Expr*> exprs,
                 std::unordered_map<std::string,int> colIdx)
    : child_(child), exprs_(std::move(exprs)), colIdx_(std::move(colIdx)) {}

void Project::open() {
    child_->open();
}

bool Project::next(physical::Row &outRow) {
    physical::Row inRow;
    if (!child_->next(inRow)) return false;
    outRow.clear();
    for (auto *e : exprs_) {
        FieldValue v = ExpressionEvaluator::eval(e, inRow, colIdx_);
        outRow.push_back(std::move(v));
    }
    return true;
}

void Project::close() {
    child_->close();
}