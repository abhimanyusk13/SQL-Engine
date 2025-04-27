// File: Catalog.h
#pragma once
#include <string>
#include <unordered_map>
#include "Schema.h"

class Catalog {
public:
    // Load metadata from disk
    void load(const std::string &path);
    // Get table schema
    Schema getTable(const std::string &name);
private:
    std::unordered_map<std::string, Schema> tables_;
};

// File: Catalog.cpp
#include "Catalog.h"

void Catalog::load(const std::string &path) {
    // TODO: read catalog files
}

Schema Catalog::getTable(const std::string &name) {
    return tables_.at(name);
}
