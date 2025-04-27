// File: Binder.h
#pragma once
#include "AST.h"

class Binder {
public:
    // Resolve names, types, fill catalog references
    void bind(AST &ast);
};

// File: Binder.cpp
#include "Binder.h"

void Binder::bind(AST &ast) {
    // TODO: name resolution, type checking
}
