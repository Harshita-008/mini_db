#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include "common/types.h"

namespace minidb {

// ============================================================
// AST Node Types
// ============================================================

enum class ASTNodeType {
    // Statements
    CREATE_TABLE,
    INSERT,
    SELECT,
    DELETE_STMT,

    // Expressions
    LITERAL,
    COLUMN_REF,
    BINARY_OP,
    STAR,         // SELECT *
};

enum class BinaryOpType {
    EQ,       // =
    NEQ,      // !=
    LT,       // <
    GT,       // >
    LTE,      // <=
    GTE,      // >=
    AND,
    OR,
};

// Forward declarations
struct ASTNode;
using ASTNodePtr = std::shared_ptr<ASTNode>;

// ============================================================
// Expression Nodes
// ============================================================

struct Expression {
    virtual ~Expression() = default;
};
using ExprPtr = std::shared_ptr<Expression>;

struct LiteralExpr : Expression {
    Value value;
    LiteralExpr(const Value& v) : value(v) {}
};

struct ColumnRefExpr : Expression {
    std::string table_name;  // Optional: for qualified references (t.col)
    std::string column_name;
    ColumnRefExpr(const std::string& col) : column_name(col) {}
    ColumnRefExpr(const std::string& table, const std::string& col)
        : table_name(table), column_name(col) {}
};

struct StarExpr : Expression {};

struct BinaryOpExpr : Expression {
    BinaryOpType op;
    ExprPtr left;
    ExprPtr right;
    BinaryOpExpr(BinaryOpType op, ExprPtr l, ExprPtr r)
        : op(op), left(std::move(l)), right(std::move(r)) {}
};

// ============================================================
// Column Definition (for CREATE TABLE)
// ============================================================

struct ColumnDef {
    std::string name;
    ColumnType type;
    uint16_t max_length = 0;
    bool is_primary_key = false;
};

// ============================================================
// Join Clause
// ============================================================

struct JoinClause {
    std::string table_name;
    ExprPtr on_condition;
};

// ============================================================
// Statement Nodes
// ============================================================

struct CreateTableStmt {
    std::string table_name;
    std::vector<ColumnDef> columns;
};

struct InsertStmt {
    std::string table_name;
    std::vector<std::vector<ExprPtr>> values_list;  // Support multiple rows
};

struct SelectStmt {
    std::vector<ExprPtr> select_list;       // Columns to select (or StarExpr)
    std::vector<std::string> from_tables;   // FROM tables
    std::vector<JoinClause> joins;          // JOIN clauses
    ExprPtr where_clause;                   // WHERE condition (optional)
};

struct DeleteStmt {
    std::string table_name;
    ExprPtr where_clause;  // WHERE condition (optional)
};

// ============================================================
// Top-level AST Node
// ============================================================

struct ASTNode {
    ASTNodeType type;
    std::variant<CreateTableStmt, InsertStmt, SelectStmt, DeleteStmt> stmt;
};

}  // namespace minidb
