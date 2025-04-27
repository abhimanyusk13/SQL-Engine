// File: Executor.cpp
#include "Executor.h"

Executor::Executor(StorageEngine &store) : store_(store) {}

std::vector<std::vector<std::string>> Executor::execute(const AST &ast) {
    // TODO: Walk plan tree, call storage engine
    return {};
}
