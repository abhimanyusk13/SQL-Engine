// File: Parser.cpp
#include "Parser.h"
#include <cctype>
#include <stdexcept>
#include <unordered_map>

// ----- Tokenization -----

void Parser::tokenize(const std::string &s) {
    static std::unordered_map<std::string, TokenType> keywords = {
        {"SELECT", TokenType::SELECT},
        {"FROM",   TokenType::FROM},
        {"WHERE",  TokenType::WHERE},
        {"INSERT", TokenType::INSERT},
        {"INTO",   TokenType::INTO},
        {"VALUES", TokenType::VALUES},
        {"UPDATE", TokenType::UPDATE},
        {"SET",    TokenType::SET},
        {"DELETE", TokenType::DELETE},
        {"CREATE", TokenType::CREATE},
        {"TABLE",  TokenType::TABLE},
        {"BEGIN",  TokenType::BEGIN},
        {"COMMIT", TokenType::COMMIT},
        {"AND",    TokenType::AND},
        {"OR",     TokenType::OR}
    };

    tokens_.clear(); pos_ = 0;
    size_t i = 0, n = s.size();
    while (i < n) {
        char c = s[i];
        if (isspace(c)) { ++i; continue; }
        // Identifiers or keywords
        if (isalpha(c) || c == '_') {
            size_t j = i+1;
            while (j < n && (isalnum(s[j])||s[j]=='_')) ++j;
            std::string w = s.substr(i, j-i);
            std::string W; for(char x:w) W+=toupper(x);
            TokenType tt = TokenType::IDENT;
            if (keywords.count(W)) tt = keywords[W];
            tokens_.push_back({tt, w});
            i = j;
            continue;
        }
        // Numbers
        if (isdigit(c)) {
            size_t j = i+1;
            while (j<n && isdigit(s[j])) ++j;
            tokens_.push_back({TokenType::INT_LITERAL, s.substr(i,j-i)});
            i = j;
            continue;
        }
        // String literals '...'
        if (c == '\'') {
            size_t j = i+1;
            while (j<n && s[j] != '\'') {
                if (s[j] == '\\' && j+1<n) j+=2;
                else ++j;
            }
            if (j>=n) throw std::runtime_error("Unterminated string literal");
            std::string lit = s.substr(i+1, j-i-1);
            tokens_.push_back({TokenType::STR_LITERAL, lit});
            i = j+1;
            continue;
        }
        // Two-char operators
        if (i+1 < n) {
            std::string two = s.substr(i,2);
            if (two == "<>" ) { tokens_.push_back({TokenType::NEQ, two}); i+=2; continue; }
            if (two == "<=" ) { tokens_.push_back({TokenType::LTE, two}); i+=2; continue; }
            if (two == ">=" ) { tokens_.push_back({TokenType::GTE, two}); i+=2; continue; }
        }
        // Single-char tokens
        switch(c) {
            case ',': tokens_.push_back({TokenType::COMMA, ","}); break;
            case ';': tokens_.push_back({TokenType::SEMICOLON, ";"}); break;
            case '(': tokens_.push_back({TokenType::LPAREN, "("}); break;
            case ')': tokens_.push_back({TokenType::RPAREN, ")"}); break;
            case '=': tokens_.push_back({TokenType::EQ, "="}); break;
            case '<': tokens_.push_back({TokenType::LT, "<"}); break;
            case '>': tokens_.push_back({TokenType::GT, ">"}); break;
            case '+': tokens_.push_back({TokenType::PLUS, "+"}); break;
            case '-': tokens_.push_back({TokenType::MINUS, "-"}); break;
            case '*': tokens_.push_back({TokenType::STAR, "*"}); break;
            case '/': tokens_.push_back({TokenType::SLASH, "/"}); break;
            case '.': tokens_.push_back({TokenType::DOT, "."}); break;
            default:
                throw std::runtime_error(std::string("Unknown char: ")+c);
        }
        ++i;
    }
    tokens_.push_back({TokenType::END, ""});
}

Parser::Token Parser::nextToken() {
    if (pos_ >= tokens_.size()) return tokens_.back();
    return tokens_[pos_++];
}

const Parser::Token &Parser::peek() const {
    if (pos_ >= tokens_.size()) return tokens_.back();
    return tokens_[pos_];
}

bool Parser::match(TokenType t) {
    if (peek().type == t) { ++pos_; return true; }
    return false;
}

bool Parser::expect(TokenType t, const std::string &errMsg) {
    if (match(t)) return true;
    throw std::runtime_error(errMsg);
}

// ----- Parsing -----

AST Parser::parse(const std::string &sqlText) {
    tokenize(sqlText);
    AST ast = parseStatement();
    return ast;
}

AST Parser::parseStatement() {
    AST ast;
    if (match(TokenType::SELECT))      { pos_--; parseSelect(ast); ast.stmtType = StmtType::SELECT; }
    else if (match(TokenType::INSERT)) { pos_--; parseInsert(ast); ast.stmtType = StmtType::INSERT; }
    else if (match(TokenType::UPDATE)) { pos_--; parseUpdate(ast); ast.stmtType = StmtType::UPDATE; }
    else if (match(TokenType::DELETE)) { pos_--; parseDelete(ast); ast.stmtType = StmtType::DELETE; }
    else if (match(TokenType::CREATE)) { pos_--; parseCreate(ast); ast.stmtType = StmtType::CREATE_TABLE; }
    else if (match(TokenType::BEGIN))  { pos_--; parseBegin(ast);  ast.stmtType = StmtType::BEGIN; }
    else if (match(TokenType::COMMIT)) { pos_--; parseCommit(ast); ast.stmtType = StmtType::COMMIT; }
    else throw std::runtime_error("Unknown statement type");
    match(TokenType::SEMICOLON); // optional
    return ast;
}

void Parser::parseSelect(AST &ast) {
    // SELECT
    expect(TokenType::SELECT, "Expected SELECT");
    ast.select = std::make_unique<SelectStmt>();
    // select list
    do {
        if (match(TokenType::STAR)) {
            // * wildcard
            auto e = std::make_unique<Expr>(); e->type = Expr::Type::COLUMN_REF; e->columnName = "*";
            ast.select->selectList.push_back(std::move(e));
        } else {
            ast.select->selectList.push_back(parseExpression());
        }
    } while (match(TokenType::COMMA));
    // FROM
    expect(TokenType::FROM, "Expected FROM");
    // table list
    do {
        if (peek().type != TokenType::IDENT)
            throw std::runtime_error("Expected table name");
        ast.select->tables.push_back(nextToken().text);
    } while (match(TokenType::COMMA));
    // optional WHERE
    if (match(TokenType::WHERE)) {
        ast.select->whereClause = parseExpression();
    }
}

void Parser::parseInsert(AST &ast) {
    expect(TokenType::INSERT, "Expected INSERT");
    expect(TokenType::INTO,   "Expected INTO");
    ast.insert = std::make_unique<InsertStmt>();
    if (peek().type != TokenType::IDENT)
        throw std::runtime_error("Expected table name after INSERT INTO");
    ast.insert->table = nextToken().text;
    // optional column list
    if (match(TokenType::LPAREN)) {
        do {
            if (peek().type != TokenType::IDENT)
                throw std::runtime_error("Expected column name");
            ast.insert->columns.push_back(nextToken().text);
        } while (match(TokenType::COMMA));
        expect(TokenType::RPAREN, "Expected ) after column list");
    }
    // VALUES
    expect(TokenType::VALUES, "Expected VALUES");
    expect(TokenType::LPAREN, "Expected ( before VALUES list");
    do {
        ast.insert->values.push_back(parseExpression());
    } while (match(TokenType::COMMA));
    expect(TokenType::RPAREN, "Expected ) after VALUES list");
}

void Parser::parseUpdate(AST &ast) {
    expect(TokenType::UPDATE, "Expected UPDATE");
    ast.update = std::make_unique<UpdateStmt>();
    if (peek().type != TokenType::IDENT)
        throw std::runtime_error("Expected table name after UPDATE");
    ast.update->table = nextToken().text;
    expect(TokenType::SET, "Expected SET");
    do {
        if (peek().type != TokenType::IDENT)
            throw std::runtime_error("Expected column name in SET");
        std::string col = nextToken().text;
        expect(TokenType::EQ, "Expected = in SET");
        auto expr = parseExpression();
        ast.update->assignments.emplace_back(col, std::move(expr));
    } while (match(TokenType::COMMA));
    if (match(TokenType::WHERE)) {
        ast.update->whereClause = parseExpression();
    }
}

void Parser::parseDelete(AST &ast) {
    expect(TokenType::DELETE, "Expected DELETE");
    expect(TokenType::FROM,   "Expected FROM");
    ast.deleteStmt = std::make_unique<DeleteStmt>();
    if (peek().type != TokenType::IDENT)
        throw std::runtime_error("Expected table name after DELETE FROM");
    ast.deleteStmt->table = nextToken().text;
    if (match(TokenType::WHERE)) {
        ast.deleteStmt->whereClause = parseExpression();
    }
}

void Parser::parseCreate(AST &ast) {
    expect(TokenType::CREATE, "Expected CREATE");
    expect(TokenType::TABLE,  "Expected TABLE");
    ast.createTable = std::make_unique<CreateTableStmt>();
    if (peek().type != TokenType::IDENT)
        throw std::runtime_error("Expected table name in CREATE TABLE");
    ast.createTable->table = nextToken().text;
    expect(TokenType::LPAREN, "Expected ( after table name");
    do {
        if (peek().type != TokenType::IDENT)
            throw std::runtime_error("Expected column name in CREATE TABLE");
        std::string colName = nextToken().text;
        if (peek().type != TokenType::IDENT)
            throw std::runtime_error("Expected data type for column");
        std::string typeName = nextToken().text;
        int length = 0;
        if (typeName == "STRING" && match(TokenType::LPAREN)) {
            if (peek().type != TokenType::INT_LITERAL)
                throw std::runtime_error("Expected length in STRING(...)");
            length = std::stoi(nextToken().text);
            expect(TokenType::RPAREN, "Expected ) after STRING length");
        }
        ast.createTable->columns.emplace_back(colName, typeName, length);
    } while (match(TokenType::COMMA));
    expect(TokenType::RPAREN, "Expected ) after CREATE TABLE columns");
}

void Parser::parseBegin(AST &ast) {
    expect(TokenType::BEGIN, "Expected BEGIN");
    ast.begin = std::make_unique<BeginStmt>();
}

void Parser::parseCommit(AST &ast) {
    expect(TokenType::COMMIT, "Expected COMMIT");
    ast.commit = std::make_unique<CommitStmt>();
}

// ----- Expression Parsing (Precedence Climbing) -----

std::unique_ptr<Expr> Parser::parseExpression() {
    auto lhs = parsePrimary();
    while (true) {
        const Token &tok = peek();
        std::string op;
        int prec = 0;
        switch (tok.type) {
            case TokenType::EQ:  op = "="; break;
            case TokenType::LT:  op = "<"; break;
            case TokenType::GT:  op = ">"; break;
            case TokenType::LTE: op = "<="; break;
            case TokenType::GTE: op = ">="; break;
            case TokenType::NEQ: op = "<>"; break;
            case TokenType::AND: op = "AND"; break;
            case TokenType::OR:  op = "OR"; break;
            default: return lhs;
        }
        prec = getPrecedence(op);
        nextToken(); // consume operator
        auto rhs = parseExpression();
        auto node = std::make_unique<Expr>();
        node->type = Expr::Type::BINARY_OP;
        node->op = op;
        node->left = std::move(lhs);
        node->right = std::move(rhs);
        lhs = std::move(node);
    }
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    const Token &tok = peek();
    if (tok.type == TokenType::IDENT) {
        // column reference
        auto node = std::make_unique<Expr>();
        node->type = Expr::Type::COLUMN_REF;
        node->columnName = nextToken().text;
        return node;
    }
    if (tok.type == TokenType::INT_LITERAL) {
        auto node = std::make_unique<Expr>();
        node->type = Expr::Type::INT_LITERAL;
        node->intValue = std::stoi(nextToken().text);
        return node;
    }
    if (tok.type == TokenType::STR_LITERAL) {
        auto node = std::make_unique<Expr>();
        node->type = Expr::Type::STR_LITERAL;
        node->strValue = nextToken().text;
        return node;
    }
    if (match(TokenType::LPAREN)) {
        auto node = parseExpression();
        expect(TokenType::RPAREN, "Expected )");
        return node;
    }

    if (currentToken().type == TokenType::IDENTIFIER &&
        peekToken().text == "(") {
        // function call
        string fname = currentToken().text;
        consume(); // identifier
        consume(); // "("
        vector<Expr*> args;
        if (currentToken().text != ")") {
            do {
                args.push_back(parseExpression());
                if (currentToken().text == ",") consume();
                else break;
            } while (true);
        }
        consume(); // ")"
        auto *e = new Expr();
        e->type = Expr::Type::FUNCTION_CALL;
        e->functionName = fname;
        e->args = std::move(args);
        return e;
    }

    throw std::runtime_error("Unexpected token in expression: " + tok.text);
}

int Parser::getPrecedence(const std::string &op) {
    if (op == "AND") return 1;
    if (op == "OR")  return 1;
    if (op == "=" || op == "<>" || op == "<" || op == ">" || op == "<=" || op == ">=") return 2;
    return 0;
}
