#include "ExpressionEvaluator.h"
#include <stdexcept>
#include <algorithm>

FieldValue ExpressionEvaluator::eval(const Expr *expr,
                                     const physical::Row &row,
                                     const std::unordered_map<std::string,int> &colIdx) {
    using PV = physical::Row;
    switch (expr->type) {
        case Expr::Type::INT_LITERAL:
            return expr->intValue;
        case Expr::Type::STR_LITERAL:
            return expr->strValue;
        case Expr::Type::COLUMN_REF: {
            auto it = colIdx.find(expr->columnName);
            if (it == colIdx.end())
                throw std::runtime_error("Unknown column in eval: " + expr->columnName);
            return row[it->second];
        }
        case Expr::Type::BINARY_OP: {
            FieldValue l = eval(expr->left.get(), row, colIdx);
            FieldValue r = eval(expr->right.get(), row, colIdx);
            const std::string &op = expr->op;
            // INT ops
            if (std::holds_alternative<int32_t>(l) && std::holds_alternative<int32_t>(r)) {
                int32_t lv = std::get<int32_t>(l);
                int32_t rv = std::get<int32_t>(r);
                if (op == "+") return lv + rv;
                if (op == "-") return lv - rv;
                if (op == "*") return lv * rv;
                if (op == "/") return lv / rv;
                if (op == "=")  return (int32_t)(lv == rv);
                if (op == "<>") return (int32_t)(lv != rv);
                if (op == "<")  return (int32_t)(lv < rv);
                if (op == ">")  return (int32_t)(lv > rv);
                if (op == "<=") return (int32_t)(lv <= rv);
                if (op == ">=") return (int32_t)(lv >= rv);
            }
            // STRING ops or mixed
            if (std::holds_alternative<std::string>(l) && std::holds_alternative<std::string>(r)) {
                const auto &ls = std::get<std::string>(l);
                const auto &rs = std::get<std::string>(r);
                if (op == "=")  return (int32_t)(ls == rs);
                if (op == "<>") return (int32_t)(ls != rs);
                if (op == "<")  return (int32_t)(ls < rs);
                if (op == ">")  return (int32_t)(ls > rs);
                if (op == "<=") return (int32_t)(ls <= rs);
                if (op == ">=") return (int32_t)(ls >= rs);
            }
            // AND/OR boolean (treated as int)
            if (op == "AND") return (int32_t)(evalBoolean(expr->left.get(), row, colIdx) && evalBoolean(expr->right.get(), row, colIdx));
            if (op == "OR")  return (int32_t)(evalBoolean(expr->left.get(), row, colIdx) || evalBoolean(expr->right.get(), row, colIdx));
            throw std::runtime_error("Unsupported operator in eval: " + op);
        }
        default:
            throw std::runtime_error("Unsupported expr type in eval");
    }
}

bool ExpressionEvaluator::evalBoolean(const Expr *expr,
                                       const physical::Row &row,
                                       const std::unordered_map<std::string,int> &colIdx) {
    FieldValue v = eval(expr, row, colIdx);
    if (std::holds_alternative<int32_t>(v))
        return std::get<int32_t>(v) != 0;
    throw std::runtime_error("Non-boolean result in predicate eval");
}