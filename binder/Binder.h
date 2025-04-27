// File: Binder.h
#pragma once

#include "AST.h"
#include "Catalog.h"
#include <vector>

class Binder {
public:
    explicit Binder(Catalog &catalog);
    // Performs name resolution and semantic checks on the AST
    void bind(AST &ast);

private:
    Catalog &catalog_;

    void bindSelect(SelectStmt *stmt);
    void bindInsert(InsertStmt *stmt);
    void bindUpdate(UpdateStmt *stmt);
    void bindDelete(DeleteStmt *stmt);

    void expandStar(SelectStmt *stmt, const std::vector<std::string> &tables);
    void resolveColumnsInExpr(Expr *expr, const std::vector<std::string> &tables);
    void typeCheckExpr(Expr *expr, const std::vector<std::string> &tables);

    // Resolves a column name, possibly qualified as table.column
    // Returns "table.column" and throws on ambiguity or missing
    std::string qualifyColumn(const std::string &col, const std::vector<std::string> &tables);
};