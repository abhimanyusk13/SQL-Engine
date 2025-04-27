// File: Executor.h
#pragma once

#include <vector>
#include "PhysicalOperator.h"

namespace executor {
    using Row = physical::Row;

    class Executor {
    public:
        // Execute a physical operator tree and collect all result rows
        static std::vector<Row> execute(physical::PhysicalOperator *root) {
            std::vector<Row> results;
            root->open();
            Row row;
            while (root->next(row)) {
                results.push_back(row);
            }
            root->close();
            return results;
        }
    };
} // namespace executor