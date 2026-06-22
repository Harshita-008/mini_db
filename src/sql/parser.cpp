#include "sql/parser.h"
#include <stdexcept>

namespace minidb {

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0), in_values_context_(false) {}

std::shared_ptr<ASTNode> Parser::Parse(Status& status) {
    if (IsAtEnd()) {
        status = Status::ParseError("Empty input");
        return nullptr;
    }

    const Token& tok = Current();

    std::shared_ptr<ASTNode> result;

    switch (tok.type) {
        case TokenType::CREATE:
            result = ParseCreateTable(status);
            break;
        case TokenType::INSERT:
            result = ParseInsert(status);
            break;
        case TokenType::SELECT:
            result = ParseSelect(status);
            break;
        case TokenType::DELETE:
            result = ParseDelete(status);
            break;
        default:
            status = Status::ParseError("Unexpected token: " + tok.value);
            return nullptr;
    }

    // Optional semicolon
    Match(TokenType::SEMICOLON);

    return result;
}

// CREATE TABLE name (col1 type, col2 type, ...)
std::shared_ptr<ASTNode> Parser::ParseCreateTable(Status& status) {
    Advance(); // skip CREATE
    if (!Expect(TokenType::TABLE, status, "Expected TABLE after CREATE")) return nullptr;

    if (Current().type != TokenType::IDENTIFIER) {
        status = Status::ParseError("Expected table name");
        return nullptr;
    }
    std::string table_name = Current().value;
    Advance();

    if (!Expect(TokenType::LPAREN, status, "Expected '(' after table name")) return nullptr;

    CreateTableStmt stmt;
    stmt.table_name = table_name;

    while (!IsAtEnd() && Current().type != TokenType::RPAREN) {
        ColumnDef col;

        if (Current().type != TokenType::IDENTIFIER) {
            status = Status::ParseError("Expected column name");
            return nullptr;
        }
        col.name = Current().value;
        Advance();

        // Parse type
        switch (Current().type) {
            case TokenType::KW_INT:
                col.type = ColumnType::INT;
                Advance();
                break;
            case TokenType::KW_FLOAT:
                col.type = ColumnType::FLOAT;
                Advance();
                break;
            case TokenType::KW_VARCHAR:
                col.type = ColumnType::VARCHAR;
                Advance();
                if (Match(TokenType::LPAREN)) {
                    if (Current().type == TokenType::INT_LITERAL) {
                        col.max_length = static_cast<uint16_t>(std::stoi(Current().value));
                        Advance();
                    }
                    Expect(TokenType::RPAREN, status, "Expected ')' after VARCHAR length");
                } else {
                    col.max_length = 255;
                }
                break;
            case TokenType::KW_BOOL:
                col.type = ColumnType::BOOL;
                Advance();
                break;
            default:
                status = Status::ParseError("Expected column type, got: " + Current().value);
                return nullptr;
        }

        // Check for PRIMARY KEY
        if (Current().type == TokenType::PRIMARY) {
            Advance();
            if (!Expect(TokenType::KEY, status, "Expected KEY after PRIMARY")) return nullptr;
            col.is_primary_key = true;
        }

        stmt.columns.push_back(col);

        if (!Match(TokenType::COMMA)) break;
    }

    if (!Expect(TokenType::RPAREN, status, "Expected ')' at end of column list")) return nullptr;

    auto node = std::make_shared<ASTNode>();
    node->type = ASTNodeType::CREATE_TABLE;
    node->stmt = std::move(stmt);
    return node;
}

// INSERT INTO table VALUES (v1, v2, ...) [, (v1, v2, ...)]
std::shared_ptr<ASTNode> Parser::ParseInsert(Status& status) {
    Advance(); // skip INSERT
    if (!Expect(TokenType::INTO, status, "Expected INTO after INSERT")) return nullptr;

    if (Current().type != TokenType::IDENTIFIER) {
        status = Status::ParseError("Expected table name");
        return nullptr;
    }
    std::string table_name = Current().value;
    Advance();

    if (!Expect(TokenType::VALUES, status, "Expected VALUES")) return nullptr;

    InsertStmt stmt;
    stmt.table_name = table_name;

    // Parse one or more value lists
    do {
        if (!Expect(TokenType::LPAREN, status, "Expected '(' before values")) return nullptr;

        std::vector<ExprPtr> values;
        in_values_context_ = true;
        while (!IsAtEnd() && Current().type != TokenType::RPAREN) {
            auto expr = ParsePrimary(status);
            if (!status.ok()) { in_values_context_ = false; return nullptr; }
            values.push_back(expr);
            if (!Match(TokenType::COMMA)) break;
        }
        in_values_context_ = false;

        if (!Expect(TokenType::RPAREN, status, "Expected ')' after values")) return nullptr;
        stmt.values_list.push_back(std::move(values));
    } while (Match(TokenType::COMMA));

    auto node = std::make_shared<ASTNode>();
    node->type = ASTNodeType::INSERT;
    node->stmt = std::move(stmt);
    return node;
}

// SELECT select_list FROM table [JOIN table ON cond] [WHERE cond]
std::shared_ptr<ASTNode> Parser::ParseSelect(Status& status) {
    Advance(); // skip SELECT

    SelectStmt stmt;

    // Parse select list
    if (Current().type == TokenType::STAR) {
        stmt.select_list.push_back(std::make_shared<StarExpr>());
        Advance();
    } else {
        while (true) {
            // Could be table.column or just column
            if (Current().type == TokenType::IDENTIFIER) {
                std::string name1 = Current().value;
                Advance();
                if (Match(TokenType::DOT)) {
                    if (Current().type != TokenType::IDENTIFIER) {
                        status = Status::ParseError("Expected column name after '.'");
                        return nullptr;
                    }
                    stmt.select_list.push_back(
                        std::make_shared<ColumnRefExpr>(name1, Current().value));
                    Advance();
                } else {
                    stmt.select_list.push_back(std::make_shared<ColumnRefExpr>(name1));
                }
            } else {
                status = Status::ParseError("Expected column name in SELECT list");
                return nullptr;
            }
            if (!Match(TokenType::COMMA)) break;
        }
    }

    // FROM clause
    if (!Expect(TokenType::FROM, status, "Expected FROM")) return nullptr;

    if (Current().type != TokenType::IDENTIFIER) {
        status = Status::ParseError("Expected table name after FROM");
        return nullptr;
    }
    stmt.from_tables.push_back(Current().value);
    Advance();

    // JOIN clauses
    while (Current().type == TokenType::JOIN) {
        Advance(); // skip JOIN

        JoinClause join;
        if (Current().type != TokenType::IDENTIFIER) {
            status = Status::ParseError("Expected table name after JOIN");
            return nullptr;
        }
        join.table_name = Current().value;
        Advance();

        if (!Expect(TokenType::ON, status, "Expected ON after JOIN table")) return nullptr;
        join.on_condition = ParseExpression(status);
        if (!status.ok()) return nullptr;

        stmt.joins.push_back(std::move(join));
    }

    // WHERE clause
    if (Match(TokenType::WHERE)) {
        stmt.where_clause = ParseExpression(status);
        if (!status.ok()) return nullptr;
    }

    auto node = std::make_shared<ASTNode>();
    node->type = ASTNodeType::SELECT;
    node->stmt = std::move(stmt);
    return node;
}

// DELETE FROM table [WHERE cond]
std::shared_ptr<ASTNode> Parser::ParseDelete(Status& status) {
    Advance(); // skip DELETE
    if (!Expect(TokenType::FROM, status, "Expected FROM after DELETE")) return nullptr;

    if (Current().type != TokenType::IDENTIFIER) {
        status = Status::ParseError("Expected table name");
        return nullptr;
    }
    std::string table_name = Current().value;
    Advance();

    DeleteStmt stmt;
    stmt.table_name = table_name;

    if (Match(TokenType::WHERE)) {
        stmt.where_clause = ParseExpression(status);
        if (!status.ok()) return nullptr;
    }

    auto node = std::make_shared<ASTNode>();
    node->type = ASTNodeType::DELETE_STMT;
    node->stmt = std::move(stmt);
    return node;
}

// Expression parsing (precedence climbing)
ExprPtr Parser::ParseExpression(Status& status) {
    return ParseOrExpr(status);
}

ExprPtr Parser::ParseOrExpr(Status& status) {
    auto left = ParseAndExpr(status);
    if (!status.ok()) return nullptr;

    while (Current().type == TokenType::OR) {
        Advance();
        auto right = ParseAndExpr(status);
        if (!status.ok()) return nullptr;
        left = std::make_shared<BinaryOpExpr>(BinaryOpType::OR, left, right);
    }
    return left;
}

ExprPtr Parser::ParseAndExpr(Status& status) {
    auto left = ParseComparison(status);
    if (!status.ok()) return nullptr;

    while (Current().type == TokenType::AND) {
        Advance();
        auto right = ParseComparison(status);
        if (!status.ok()) return nullptr;
        left = std::make_shared<BinaryOpExpr>(BinaryOpType::AND, left, right);
    }
    return left;
}

ExprPtr Parser::ParseComparison(Status& status) {
    auto left = ParsePrimary(status);
    if (!status.ok()) return nullptr;

    BinaryOpType op;
    bool has_op = true;
    switch (Current().type) {
        case TokenType::EQ:  op = BinaryOpType::EQ; break;
        case TokenType::NEQ: op = BinaryOpType::NEQ; break;
        case TokenType::LT:  op = BinaryOpType::LT; break;
        case TokenType::GT:  op = BinaryOpType::GT; break;
        case TokenType::LTE: op = BinaryOpType::LTE; break;
        case TokenType::GTE: op = BinaryOpType::GTE; break;
        default: has_op = false; break;
    }

    if (has_op) {
        Advance();
        auto right = ParsePrimary(status);
        if (!status.ok()) return nullptr;
        return std::make_shared<BinaryOpExpr>(op, left, right);
    }

    return left;
}

ExprPtr Parser::ParsePrimary(Status& status) {
    const Token& tok = Current();

    switch (tok.type) {
        case TokenType::INT_LITERAL: {
            int32_t val = std::stoi(tok.value);
            Advance();
            return std::make_shared<LiteralExpr>(Value(val));
        }
        case TokenType::FLOAT_LITERAL: {
            double val = std::stod(tok.value);
            Advance();
            return std::make_shared<LiteralExpr>(Value(val));
        }
        case TokenType::STRING_LITERAL: {
            std::string val = tok.value;
            Advance();
            return std::make_shared<LiteralExpr>(Value(val));
        }
        case TokenType::IDENTIFIER: {
            std::string name1 = tok.value;
            Advance();
            // In VALUES context, treat bare identifiers as string literals
            if (in_values_context_) {
                return std::make_shared<LiteralExpr>(Value(name1));
            }
            if (Match(TokenType::DOT)) {
                if (Current().type != TokenType::IDENTIFIER) {
                    status = Status::ParseError("Expected column name after '.'");
                    return nullptr;
                }
                std::string name2 = Current().value;
                Advance();
                return std::make_shared<ColumnRefExpr>(name1, name2);
            }
            return std::make_shared<ColumnRefExpr>(name1);
        }
        case TokenType::LPAREN: {
            Advance();
            auto expr = ParseExpression(status);
            if (!status.ok()) return nullptr;
            if (!Expect(TokenType::RPAREN, status, "Expected ')'")) return nullptr;
            return expr;
        }
        default:
            status = Status::ParseError("Unexpected token: " + tok.value);
            return nullptr;
    }
}

const Token& Parser::Current() const {
    static Token eof(TokenType::END_OF_INPUT, "", 0, 0);
    if (pos_ >= tokens_.size()) return eof;
    return tokens_[pos_];
}

const Token& Parser::Peek() const {
    static Token eof(TokenType::END_OF_INPUT, "", 0, 0);
    if (pos_ + 1 >= tokens_.size()) return eof;
    return tokens_[pos_ + 1];
}

bool Parser::Match(TokenType type) {
    if (Current().type == type) {
        Advance();
        return true;
    }
    return false;
}

bool Parser::Expect(TokenType type, Status& status, const std::string& msg) {
    if (!Match(type)) {
        status = Status::ParseError(msg + ", got: " + Current().value);
        return false;
    }
    return true;
}

void Parser::Advance() {
    if (pos_ < tokens_.size()) pos_++;
}

bool Parser::IsAtEnd() const {
    return pos_ >= tokens_.size() || tokens_[pos_].type == TokenType::END_OF_INPUT;
}

}  // namespace minidb
