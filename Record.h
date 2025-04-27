// File: Record.h
#pragma once

#include "Schema.h"
#include <variant>
#include <vector>
#include <string>

using FieldValue = std::variant<int32_t, std::string>;

class Record {
public:
    // Create empty record conforming to schema
    explicit Record(const Schema &schema);
    // Create record with given values (must match schema)
    Record(const Schema &schema, const std::vector<FieldValue> &values);

    // Serialize record into contiguous byte array
    std::vector<char> serialize() const;
    // Deserialize record from buffer
    static Record deserialize(const Schema &schema, const char *buffer);

    // Access values
    const std::vector<FieldValue> &getValues() const;

private:
    const Schema &schema_;
    std::vector<FieldValue> values_;
};