// File: Binder.cpp
#include "Binder.h"
#include "FunctionRegistry.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>

Binder::Binder(Catalog &catalog) : catalog_(catalog) {}

void Binder::bind(AST &ast) {
    switch (ast.stmtType) {
        case StmtType::SELECT:
            bindSelect(ast.select.get());
            break;
        case StmtType::INSERT:
            bindInsert(ast.insert.get());
            break;
        case StmtType::UPDATE:
            bindUpdate(ast.update.get());
            break;
        case StmtType::DELETE:
            bindDelete(ast.deleteStmt.get());
            break;
        default:
            // Other statements need no binding
            break;
    }
}

void Binder::bindSelect(SelectStmt *stmt) {
    if (stmt->tables.empty())
        throw std::runtime_error("SELECT requires at least one table");
    // Verify tables exist
    for (auto &t : stmt->tables) {
        catalog_.getTable(t); // throws if not found
    }
    // Expand '*' into explicit columns
    expandStar(stmt, stmt->tables);
    // Resolve columns, functions, and type-check in select list
    for (auto &e : stmt->selectList) {
        resolveColumnsInExpr(e.get(), stmt->tables);
        typeCheckExpr(e.get(), stmt->tables);
    }
    // WHERE clause
    if (stmt->whereClause) {
        resolveColumnsInExpr(stmt->whereClause.get(), stmt->tables);
        typeCheckExpr(stmt->whereClause.get(), stmt->tables);
    }
}

void Binder::bindInsert(InsertStmt *stmt) {
    // Table
    Schema schema = catalog_.getTable(stmt->table);
    size_t numCols = stmt->columns.size();
    if (numCols == 0) {
        // Default to all columns in schema order
        numCols = schema.numColumns();
        for (size_t i = 0; i < numCols; ++i) {
            stmt->columns.push_back(schema.getColumn(i).name);
        }
    }
    if (stmt->values.size() != numCols)
        throw std::runtime_error("Column count does not match VALUES count");
    // Resolve and type-check each value expression
    for (size_t i = 0; i < numCols; ++i) {
        auto &colName = stmt->columns[i];
        // Validate column exists
        Column col = catalog_.getColumnInfo(stmt->table, colName);
        Expr *e = stmt->values[i].get();
        if (e->type == Expr::Type::COLUMN_REF)
            throw std::runtime_error("INSERT VALUES must be literals or expressions, not column refs");
        // Type-check literal types against column type
        if (col.type == DataType::INT && e->type != Expr::Type::INT_LITERAL)
            throw std::runtime_error("Type mismatch for column " + colName);
        if (col.type == DataType::STRING && e->type != Expr::Type::STR_LITERAL)
            throw std::runtime_error("Type mismatch for column " + colName);
    }
}

void Binder::bindUpdate(UpdateStmt *stmt) {
    // Table
    catalog_.getTable(stmt->table);
    // Assignments
    for (auto &pr : stmt->assignments) {
        const std::string &colName = pr.first;
        Expr *e = pr.second.get();
        Column col = catalog_.getColumnInfo(stmt->table, colName);
        // Resolve column refs and function calls in e
        resolveColumnsInExpr(e, {stmt->table});
        typeCheckExpr(e, {stmt->table});
        // Type-check literal assignment
        if (e->type == Expr::Type::INT_LITERAL && col.type != DataType::INT)
            throw std::runtime_error("Type mismatch for column " + colName);
        if (e->type == Expr::Type::STR_LITERAL && col.type != DataType::STRING)
            throw std::runtime_error("Type mismatch for column " + colName);
    }
    // WHERE
    if (stmt->whereClause) {
        resolveColumnsInExpr(stmt->whereClause.get(), {stmt->table});
        typeCheckExpr(stmt->whereClause.get(), {stmt->table});
    }
}

void Binder::bindDelete(DeleteStmt *stmt) {
    // Table
    catalog_.getTable(stmt->table);
    if (stmt->whereClause) {
        resolveColumnsInExpr(stmt->whereClause.get(), {stmt->table});
        typeCheckExpr(stmt->whereClause.get(), {stmt->table});
    }
}

void Binder::expandStar(SelectStmt *stmt, const std::vector<std::string> &tables) {
    std::vector<std::unique_ptr<Expr>> newList;
    for (auto &e : stmt->selectList) {
        if (e->type == Expr::Type::COLUMN_REF && e->columnName == "*") {
            // Expand for each table
            for (auto &t : tables) {
                Schema schema = catalog_.getTable(t);
                for (size_t i = 0; i < schema.numColumns(); ++i) {
                    auto col = schema.getColumn(i).name;
                    auto ex = std::make_unique<Expr>();
                    ex->type = Expr::Type::COLUMN_REF;
                    ex->columnName = t + "." + col;
                    newList.push_back(std::move(ex));
                }
            }
        } else {
            newList.push_back(std::move(e));
        }
    }
    stmt->selectList = std::move(newList);
}

std::string Binder::qualifyColumn(const std::string &col, const std::vector<std::string> &tables) {
    // If already qualified
    auto pos = col.find('.');
    if (pos != std::string::npos) {
        std::string t = col.substr(0, pos);
        std::string c = col.substr(pos + 1);
        // Verify table and column
        catalog_.getTable(t);
        catalog_.getColumnInfo(t, c);
        return t + "." + c;
    }
    // Unqualified: search in tables
    std::string found;
    for (auto &t : tables) {
        try {
            catalog_.getColumnInfo(t, col);
            if (!found.empty())
                throw std::runtime_error("Ambiguous column: " + col);
            found = t + "." + col;
        } catch (...) {
            // not in this table
        }
    }
    if (found.empty())
        throw std::runtime_error("Column not found: " + col);
    return found;
}

void Binder::resolveColumnsInExpr(Expr *expr, const std::vector<std::string> &tables) {
    switch (expr->type) {
      case Expr::Type::COLUMN_REF:
        expr->columnName = qualifyColumn(expr->columnName, tables);
        break;
      case Expr::Type::BINARY_OP:
        resolveColumnsInExpr(expr->left.get(), tables);
        resolveColumnsInExpr(expr->right.get(), tables);
        break;
      case Expr::Type::FUNCTION_CALL:
        // Resolve each argument, then ensure function exists
        for (auto &arg : expr->args) {
            resolveColumnsInExpr(arg.get(), tables);
        }
        if (!FunctionRegistry::hasFunction(expr->functionName)) {
            throw std::runtime_error("Unknown function: " + expr->functionName);
        }
        break;
      default:
        break;
    }
}

void Binder::typeCheckExpr(Expr *expr, const std::vector<std::string> &tables) {
    switch (expr->type) {
      case Expr::Type::BINARY_OP: {
        // Recurse
        typeCheckExpr(expr->left.get(), tables);
        typeCheckExpr(expr->right.get(), tables);
        // Determine operand types
        auto getType = [&](Expr *e) -> DataType {
            if (e->type == Expr::Type::INT_LITERAL)   return DataType::INT;
            if (e->type == Expr::Type::STR_LITERAL)   return DataType::STRING;
            if (e->type == Expr::Type::COLUMN_REF) {
                auto pos = e->columnName.find('.');
                auto t = e->columnName.substr(0, pos);
                auto c = e->columnName.substr(pos + 1);
                return catalog_.getColumnInfo(t, c).type;
            }
            throw std::runtime_error("Invalid expression in type checking");
        };
        DataType lt = getType(expr->left.get());
        DataType rt = getType(expr->right.get());
        // Comparison ops
        if (expr->op == "=" || expr->op == "<>" ||
            expr->op == "<" || expr->op == ">" ||
            expr->op == "<="|| expr->op == ">=") {
            if (lt != rt)
                throw std::runtime_error("Type mismatch in comparison: " + expr->op);
        } else if (expr->op == "AND" || expr->op == "OR") {
            // assume comparisons are valid boolean
        } else {
            throw std::runtime_error("Unsupported operator in WHERE: " + expr->op);
        }
        break;
      }
      case Expr::Type::FUNCTION_CALL:
        // Type-check each argument
        for (auto &arg : expr->args) {
            // first resolve columns (already done) then type-check
            typeCheckExpr(arg.get(), tables);
        }
        // (Optionally you could validate arity/signature here,
        //  but we rely on the UDF itself to throw on bad args.)
        break;
      default:
        break;
    }
}
