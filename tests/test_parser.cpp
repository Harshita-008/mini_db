#include <gtest/gtest.h>
#include "sql/lexer.h"
#include "sql/parser.h"
#include "sql/ast.h"

using namespace minidb;

TEST(ParserTest, CreateTable) {
    Lexer lexer("CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(255), age INT)");
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);
    Status status;
    auto ast = parser.Parse(status);
    ASSERT_TRUE(status.ok()) << status.message();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, ASTNodeType::CREATE_TABLE);

    auto& stmt = std::get<CreateTableStmt>(ast->stmt);
    EXPECT_EQ(stmt.table_name, "users");
    EXPECT_EQ(stmt.columns.size(), 3);
    EXPECT_EQ(stmt.columns[0].name, "id");
    EXPECT_TRUE(stmt.columns[0].is_primary_key);
    EXPECT_EQ(stmt.columns[1].type, ColumnType::VARCHAR);
}

TEST(ParserTest, InsertStatement) {
    Lexer lexer("INSERT INTO users VALUES (1, 'Alice', 30)");
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);
    Status status;
    auto ast = parser.Parse(status);
    ASSERT_TRUE(status.ok()) << status.message();
    EXPECT_EQ(ast->type, ASTNodeType::INSERT);

    auto& stmt = std::get<InsertStmt>(ast->stmt);
    EXPECT_EQ(stmt.table_name, "users");
    EXPECT_EQ(stmt.values_list.size(), 1);
    EXPECT_EQ(stmt.values_list[0].size(), 3);
}

TEST(ParserTest, SelectStar) {
    Lexer lexer("SELECT * FROM users WHERE age > 25");
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);
    Status status;
    auto ast = parser.Parse(status);
    ASSERT_TRUE(status.ok()) << status.message();
    EXPECT_EQ(ast->type, ASTNodeType::SELECT);

    auto& stmt = std::get<SelectStmt>(ast->stmt);
    EXPECT_EQ(stmt.from_tables.size(), 1);
    EXPECT_EQ(stmt.from_tables[0], "users");
    EXPECT_NE(stmt.where_clause, nullptr);
}

TEST(ParserTest, SelectWithJoin) {
    Lexer lexer("SELECT t1.name, t2.name FROM t1 JOIN t2 ON t1.id = t2.id");
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);
    Status status;
    auto ast = parser.Parse(status);
    ASSERT_TRUE(status.ok()) << status.message();

    auto& stmt = std::get<SelectStmt>(ast->stmt);
    EXPECT_EQ(stmt.from_tables.size(), 1);
    EXPECT_EQ(stmt.joins.size(), 1);
    EXPECT_EQ(stmt.joins[0].table_name, "t2");
}

TEST(ParserTest, DeleteStatement) {
    Lexer lexer("DELETE FROM users WHERE id = 1");
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);
    Status status;
    auto ast = parser.Parse(status);
    ASSERT_TRUE(status.ok()) << status.message();
    EXPECT_EQ(ast->type, ASTNodeType::DELETE_STMT);

    auto& stmt = std::get<DeleteStmt>(ast->stmt);
    EXPECT_EQ(stmt.table_name, "users");
    EXPECT_NE(stmt.where_clause, nullptr);
}

TEST(ParserTest, ComplexWhere) {
    Lexer lexer("SELECT * FROM t WHERE age > 20 AND name = 'Alice'");
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);
    Status status;
    auto ast = parser.Parse(status);
    ASSERT_TRUE(status.ok()) << status.message();

    auto& stmt = std::get<SelectStmt>(ast->stmt);
    auto* bin = dynamic_cast<BinaryOpExpr*>(stmt.where_clause.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op, BinaryOpType::AND);
}

TEST(LexerTest, Tokenize) {
    Lexer lexer("SELECT * FROM users WHERE id = 42");
    auto tokens = lexer.Tokenize();
    EXPECT_EQ(tokens[0].type, TokenType::SELECT);
    EXPECT_EQ(tokens[1].type, TokenType::STAR);
    EXPECT_EQ(tokens[2].type, TokenType::FROM);
    EXPECT_EQ(tokens[3].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[4].type, TokenType::WHERE);
}
