// File: Parser.h
#pragma once

#include <string>
#include <vector>
#include "AST.h"

class Parser {
public:
    // Parse SQL text into an AST
    AST parse(const std::string &sqlText);

private:
    enum class TokenType {
        END,
        IDENT,            // identifiers
        INT_LITERAL,
        STR_LITERAL,
        COMMA, SEMICOLON,
        LPAREN, RPAREN,
        EQ, NEQ, LT, LTE, GT, GTE,
        PLUS, MINUS, STAR, SLASH,
        DOT,
        // Keywords
        SELECT, FROM, WHERE,
        INSERT, INTO, VALUES,
        UPDATE, SET,
        DELETE,
        CREATE, TABLE,
        BEGIN, COMMIT,
        AND, OR
    };

    struct Token {
        TokenType type;
        std::string text;
    };

    std::vector<Token> tokens_;
    size_t pos_;

    // Tokenization
    void tokenize(const std::string &s);
    Token nextToken();
    const Token &peek() const;
    bool match(TokenType t);
    bool expect(TokenType t, const std::string &errMsg);

    // Parsers for statements
    AST parseStatement();
    void parseSelect(AST &ast);
    void parseInsert(AST &ast);
    void parseUpdate(AST &ast);
    void parseDelete(AST &ast);
    void parseCreate(AST &ast);
    void parseBegin(AST &ast);
    void parseCommit(AST &ast);

    // Expression parsing (precedence climbing)
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parsePrimary();
    int getPrecedence(const std::string &op);
};
