// File: Executor.h
#pragma once
#include "AST.h"
#include "StorageEngine.h"

class Executor {
public:
    Executor(StorageEngine &store);
    // Execute bound AST, return result rows
    std::vector<std::vector<std::string>> execute(const AST &ast);
private:
    StorageEngine &store_;
};
