// File: PhysicalOperator.h
#pragma once

#include <vector>
#include "Record.h"  // for FieldValue

namespace physical {
    // A row produced by an operator: a sequence of field values
    using Row = std::vector<FieldValue>;

    // Abstract interface for all physical operators
    class PhysicalOperator {
    public:
        // Prepare the operator (e.g., allocate resources, initialize child operators)
        virtual void open() = 0;
        
        // Produce the next row. Returns true if a row was produced, false on end of stream.
        virtual bool next(Row &row) = 0;
        
        // Clean up resources (e.g., close child operators)
        virtual void close() = 0;
        
        virtual ~PhysicalOperator() = default;
    };
} // namespace physical
