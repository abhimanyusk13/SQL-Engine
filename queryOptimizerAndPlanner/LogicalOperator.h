// File: LogicalOperator.h
#pragma once

#include <string>
#include <vector>
#include "AST.h"

// Types of logical operators
enum class LogicalOpType {
    SeqScan,    // table scan
    Filter,     // selection
    Project,    // projection
    Join,       // join
    Insert,     // insert
    Update,     // update
    Delete,     // delete
    CreateTable,// create table
    Begin,      // begin transaction
    Commit      // commit transaction
};

// Base class for logical plan nodes
class LogicalOperator {
public:
    LogicalOpType opType;
    std::vector<LogicalOperator*> children;
    virtual ~LogicalOperator() {}
};

// Table scan node
class LogicalSeqScan : public LogicalOperator {
public:
    std::string tableName;
    LogicalSeqScan(const std::string &table) {
        opType = LogicalOpType::SeqScan;
        tableName = table;
    }
};

// Filter node
class LogicalFilter : public LogicalOperator {
public:
    Expr *predicate;  // pointer into AST
    LogicalFilter(Expr *pred, LogicalOperator *child) {
        opType = LogicalOpType::Filter;
        predicate = pred;
        children.push_back(child);
    }
};

// Project node
class LogicalProject : public LogicalOperator {
public:
    std::vector<Expr*> projections;  // pointers into AST
    LogicalProject(const std::vector<Expr*> &exprs, LogicalOperator *child) {
        opType = LogicalOpType::Project;
        projections = exprs;
        children.push_back(child);
    }
};

// Insert node
class LogicalInsert : public LogicalOperator {
public:
    std::string tableName;
    std::vector<std::string> columns;
    std::vector<Expr*> values;  // literal expressions
    LogicalInsert(const std::string &t,
                  const std::vector<std::string> &cols,
                  const std::vector<Expr*> &vals) {
        opType = LogicalOpType::Insert;
        tableName = t;
        columns = cols;
        values = vals;
    }
};

// Update node
class LogicalUpdate : public LogicalOperator {
public:
    std::string tableName;
    std::vector<std::pair<std::string, Expr*>> assignments;
    Expr *predicate;
    LogicalUpdate(const std::string &t,
                  const std::vector<std::pair<std::string, Expr*>> &assigns,
                  Expr *pred) {
        opType = LogicalOpType::Update;
        tableName = t;
        assignments = assigns;
        predicate = pred;
    }
};

// Delete node
class LogicalDelete : public LogicalOperator {
public:
    std::string tableName;
    Expr *predicate;
    LogicalDelete(const std::string &t, Expr *pred) {
        opType = LogicalOpType::Delete;
        tableName = t;
        predicate = pred;
    }
};

// Create table node
class LogicalCreateTable : public LogicalOperator {
public:
    std::string tableName;
    std::vector<std::tuple<std::string, std::string, int>> columns;
    LogicalCreateTable(const std::string &t,
                       const std::vector<std::tuple<std::string, std::string, int>> &cols) {
        opType = LogicalOpType::CreateTable;
        tableName = t;
        columns = cols;
    }
};

// Begin transaction
class LogicalBegin : public LogicalOperator {
public:
    LogicalBegin() { opType = LogicalOpType::Begin; }
};

// Commit transaction
class LogicalCommit : public LogicalOperator {
public:
    LogicalCommit() { opType = LogicalOpType::Commit; }
};

// Join node (cross join)
class LogicalJoin : public LogicalOperator {
public:
    LogicalJoin(LogicalOperator *left, LogicalOperator *right) {
        opType = LogicalOpType::Join;
        children.push_back(left);
        children.push_back(right);
    }
};