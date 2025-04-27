// File: Schema.h
#pragma once

#include <string>
#include <vector>
#include <cstddef>

// Supported data types
enum class DataType { INT, STRING };

// Column definition: name, type, and for STRING a fixed length
struct Column {
    std::string name;
    DataType type;
    std::size_t length; // bytes for STRING; ignored for INT
};

class Schema {
public:
    // Construct schema from list of columns
    explicit Schema(const std::vector<Column> &cols);

    // Total size in bytes of one record under this schema
    std::size_t getRecordSize() const;
    // Number of columns
    std::size_t numColumns() const;
    // Access column definitions
    const Column &getColumn(std::size_t idx) const;

private:
    std::vector<Column> columns_;
    std::size_t recordSize_;
};