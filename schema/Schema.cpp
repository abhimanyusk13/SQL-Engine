// File: Schema.cpp
#include "Schema.h"
#include <stdexcept>

Schema::Schema(const std::vector<Column> &cols)
    : columns_(cols), recordSize_(0) {
    for (const auto &col : columns_) {
        switch (col.type) {
            case DataType::INT:
                recordSize_ += sizeof(int32_t);
                break;
            case DataType::STRING:
                if (col.length == 0) {
                    throw std::runtime_error("STRING column must have positive length");
                }
                recordSize_ += col.length;
                break;
        }
    }
}

std::size_t Schema::getRecordSize() const {
    return recordSize_;
}

std::size_t Schema::numColumns() const {
    return columns_.size();
}

const Column &Schema::getColumn(std::size_t idx) const {
    return columns_.at(idx);
}