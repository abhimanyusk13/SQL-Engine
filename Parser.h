// File: Parser.h
#pragma once
#include <string>
#include "AST.h"

class Parser {
public:
    // Parse SQL text into an AST
    AST parse(const std::string &sql);
};

// File: Parser.cpp
#include "Parser.h"

AST Parser::parse(const std::string &sql) {
    // TODO: Lex -> tokens, build AST
    return AST();
}
