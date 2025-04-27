// File: AST.h
#pragma once

#include <string>
#include <vector>
#include <memory>

// Statement types
enum class StmtType { SELECT, INSERT, UPDATE, DELETE, CREATE_TABLE, BEGIN, COMMIT };

// Forward declarations
struct Expr;

// SELECT statement
struct SelectStmt {
    std::vector<std::unique_ptr<Expr>> selectList;  // expressions or '*'
    std::vector<std::string> tables;                // FROM table1, table2...
    std::unique_ptr<Expr> whereClause;              // optional WHERE
};

// INSERT statement
struct InsertStmt {
    std::string table;
    std::vector<std::string> columns;
    std::vector<std::unique_ptr<Expr>> values;
};

// UPDATE statement
struct UpdateStmt {
    std::string table;
    std::vector<std::pair<std::string, std::unique_ptr<Expr>>> assignments; // col=expr
    std::unique_ptr<Expr> whereClause;
};

// DELETE statement
struct DeleteStmt {
    std::string table;
    std::unique_ptr<Expr> whereClause;
};

// CREATE TABLE statement
struct CreateTableStmt {
    std::string table;
    // column name, type name, optional length for STRING
    std::vector<std::tuple<std::string, std::string, int>> columns;
};

// BEGIN / COMMIT statements have no extra fields
struct BeginStmt {};
struct CommitStmt {};

// Expression AST
struct Expr {
    enum class Type { COLUMN_REF, INT_LITERAL, STR_LITERAL, BINARY_OP, FUNCTION_CALL} type;
    // for COLUMN_REF
    std::string columnName;
    // for literals
    int intValue;
    std::string strValue;
    // for binary ops
    std::string op; // =, <, >, AND, OR, etc.
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    // for FUNCTION_CALL:
    std::string               functionName;
    std::vector<Expr*>        args;
};

// Root AST
struct AST {
    StmtType stmtType;
    std::unique_ptr<SelectStmt> select;
    std::unique_ptr<InsertStmt> insert;
    std::unique_ptr<UpdateStmt> update;
    std::unique_ptr<DeleteStmt> deleteStmt;
    std::unique_ptr<CreateTableStmt> createTable;
    std::unique_ptr<BeginStmt> begin;
    std::unique_ptr<CommitStmt> commit;
};