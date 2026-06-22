#pragma once

#include "sql/ast.h"
#include "sql/lexer.h"
#include "common/status.h"
#include <memory>

namespace minidb {

// Recursive descent SQL parser.
class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    // Parse a single SQL statement
    std::shared_ptr<ASTNode> Parse(Status& status);

private:
    // Statement parsers
    std::shared_ptr<ASTNode> ParseCreateTable(Status& status);
    std::shared_ptr<ASTNode> ParseInsert(Status& status);
    std::shared_ptr<ASTNode> ParseSelect(Status& status);
    std::shared_ptr<ASTNode> ParseDelete(Status& status);

    // Expression parsers
    ExprPtr ParseExpression(Status& status);
    ExprPtr ParseOrExpr(Status& status);
    ExprPtr ParseAndExpr(Status& status);
    ExprPtr ParseComparison(Status& status);
    ExprPtr ParsePrimary(Status& status);

    // Helpers
    const Token& Current() const;
    const Token& Peek() const;
    bool Match(TokenType type);
    bool Expect(TokenType type, Status& status, const std::string& msg);
    void Advance();
    bool IsAtEnd() const;

    std::vector<Token> tokens_;
    size_t pos_;
    bool in_values_context_;  // True when parsing inside VALUES (...)
};

}  // namespace minidb
