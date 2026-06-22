#pragma once

#include <string>
#include <vector>

namespace minidb {

enum class TokenType {
    // Keywords
    SELECT, FROM, WHERE, INSERT, INTO, VALUES, DELETE,
    CREATE, TABLE, JOIN, ON, AND, OR, NOT,
    PRIMARY, KEY,
    // Type keywords
    KW_INT, KW_FLOAT, KW_VARCHAR, KW_BOOL,

    // Operators
    EQ,       // =
    NEQ,      // !=
    LT,       // <
    GT,       // >
    LTE,      // <=
    GTE,      // >=

    // Punctuation
    STAR,     // *
    COMMA,    // ,
    LPAREN,   // (
    RPAREN,   // )
    SEMICOLON,// ;
    DOT,      // .

    // Literals
    INT_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,

    // Identifier
    IDENTIFIER,

    // Special
    END_OF_INPUT,
    INVALID,
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int col;

    Token() : type(TokenType::INVALID), line(0), col(0) {}
    Token(TokenType t, const std::string& v, int l = 0, int c = 0)
        : type(t), value(v), line(l), col(c) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& input);

    // Tokenize the entire input
    std::vector<Token> Tokenize();

private:
    Token NextToken();
    void SkipWhitespace();
    Token ReadIdentifierOrKeyword();
    Token ReadNumber();
    Token ReadString();

    char Peek() const;
    char Advance();
    bool IsAtEnd() const;

    std::string input_;
    size_t pos_;
    int line_;
    int col_;
};

}  // namespace minidb
