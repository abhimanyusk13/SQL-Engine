// File: Record.cpp
#include "Record.h"
#include <cstring>
#include <stdexcept>

Record::Record(const Schema &schema)
    : schema_(schema), values_(schema.numColumns()) {}

Record::Record(const Schema &schema, const std::vector<FieldValue> &values)
    : schema_(schema), values_(values) {
    if (values_.size() != schema_.numColumns()) {
        throw std::runtime_error("Value count does not match schema column count");
    }
    // Optionally: validate types and string lengths
    for (std::size_t i = 0; i < values_.size(); ++i) {
        const Column &col = schema_.getColumn(i);
        if (col.type == DataType::INT) {
            if (!std::holds_alternative<int32_t>(values_[i])) {
                throw std::runtime_error("Type mismatch: expected INT");
            }
        } else if (col.type == DataType::STRING) {
            if (!std::holds_alternative<std::string>(values_[i])) {
                throw std::runtime_error("Type mismatch: expected STRING");
            }
            const auto &s = std::get<std::string>(values_[i]);
            if (s.size() > col.length) {
                throw std::runtime_error("STRING value exceeds defined column length");
            }
        }
    }
}

std::vector<char> Record::serialize() const {
    std::size_t recSize = schema_.getRecordSize();
    std::vector<char> buffer(recSize);
    std::size_t offset = 0;

    for (std::size_t i = 0; i < values_.size(); ++i) {
        const Column &col = schema_.getColumn(i);
        if (col.type == DataType::INT) {
            int32_t v = std::get<int32_t>(values_[i]);
            std::memcpy(buffer.data() + offset, &v, sizeof(v));
            offset += sizeof(v);
        } else {
            const std::string &s = std::get<std::string>(values_[i]);
            std::size_t len = s.size();
            std::memcpy(buffer.data() + offset, s.data(), len);
            // pad remaining bytes with zeros
            std::memset(buffer.data() + offset + len, 0, col.length - len);
            offset += col.length;
        }
    }
    return buffer;
}

Record Record::deserialize(const Schema &schema, const char *buffer) {
    std::size_t offset = 0;
    std::vector<FieldValue> values;
    values.reserve(schema.numColumns());

    for (std::size_t i = 0; i < schema.numColumns(); ++i) {
        const Column &col = schema.getColumn(i);
        if (col.type == DataType::INT) {
            int32_t v;
            std::memcpy(&v, buffer + offset, sizeof(v));
            values.emplace_back(v);
            offset += sizeof(v);
        } else {
            // Read fixed-length string and remove trailing zeros
            std::string s;
            s.assign(buffer + offset, col.length);
            auto pos = s.find('\0');
            if (pos != std::string::npos) s.resize(pos);
            values.emplace_back(s);
            offset += col.length;
        }
    }

    return Record(schema, values);
}

const std::vector<FieldValue> &Record::getValues() const {
    return values_;
}