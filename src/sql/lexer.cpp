#include "sql/lexer.h"
#include <cctype>
#include <algorithm>
#include <unordered_map>

namespace minidb {

static const std::unordered_map<std::string, TokenType> keywords = {
    {"SELECT", TokenType::SELECT},
    {"FROM", TokenType::FROM},
    {"WHERE", TokenType::WHERE},
    {"INSERT", TokenType::INSERT},
    {"INTO", TokenType::INTO},
    {"VALUES", TokenType::VALUES},
    {"DELETE", TokenType::DELETE},
    {"CREATE", TokenType::CREATE},
    {"TABLE", TokenType::TABLE},
    {"JOIN", TokenType::JOIN},
    {"ON", TokenType::ON},
    {"AND", TokenType::AND},
    {"OR", TokenType::OR},
    {"NOT", TokenType::NOT},
    {"PRIMARY", TokenType::PRIMARY},
    {"KEY", TokenType::KEY},
    {"INT", TokenType::KW_INT},
    {"INTEGER", TokenType::KW_INT},
    {"FLOAT", TokenType::KW_FLOAT},
    {"DOUBLE", TokenType::KW_FLOAT},
    {"VARCHAR", TokenType::KW_VARCHAR},
    {"BOOL", TokenType::KW_BOOL},
    {"BOOLEAN", TokenType::KW_BOOL},
};

Lexer::Lexer(const std::string& input) : input_(input), pos_(0), line_(1), col_(1) {}

std::vector<Token> Lexer::Tokenize() {
    std::vector<Token> tokens;
    while (!IsAtEnd()) {
        SkipWhitespace();
        if (IsAtEnd()) break;

        Token tok = NextToken();
        if (tok.type != TokenType::INVALID) {
            tokens.push_back(tok);
        }
    }
    tokens.push_back(Token(TokenType::END_OF_INPUT, "", line_, col_));
    return tokens;
}

Token Lexer::NextToken() {
    char c = Peek();

    // Single character tokens
    switch (c) {
        case '*': Advance(); return Token(TokenType::STAR, "*", line_, col_ - 1);
        case ',': Advance(); return Token(TokenType::COMMA, ",", line_, col_ - 1);
        case '(': Advance(); return Token(TokenType::LPAREN, "(", line_, col_ - 1);
        case ')': Advance(); return Token(TokenType::RPAREN, ")", line_, col_ - 1);
        case ';': Advance(); return Token(TokenType::SEMICOLON, ";", line_, col_ - 1);
        case '.': Advance(); return Token(TokenType::DOT, ".", line_, col_ - 1);
        default: break;
    }

    // Two-character operators
    if (c == '!' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '=') {
        Advance(); Advance();
        return Token(TokenType::NEQ, "!=", line_, col_ - 2);
    }
    if (c == '<' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '=') {
        Advance(); Advance();
        return Token(TokenType::LTE, "<=", line_, col_ - 2);
    }
    if (c == '>' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '=') {
        Advance(); Advance();
        return Token(TokenType::GTE, ">=", line_, col_ - 2);
    }

    // Single-character operators
    if (c == '=') { Advance(); return Token(TokenType::EQ, "=", line_, col_ - 1); }
    if (c == '<') { Advance(); return Token(TokenType::LT, "<", line_, col_ - 1); }
    if (c == '>') { Advance(); return Token(TokenType::GT, ">", line_, col_ - 1); }

    // String literal
    if (c == '\'') return ReadString();

    // Number
    if (std::isdigit(c) || (c == '-' && pos_ + 1 < input_.size() && std::isdigit(input_[pos_ + 1]))) {
        return ReadNumber();
    }

    // Identifier or keyword
    if (std::isalpha(c) || c == '_') return ReadIdentifierOrKeyword();

    // Unknown character — skip
    Advance();
    return Token(TokenType::INVALID, std::string(1, c), line_, col_ - 1);
}

void Lexer::SkipWhitespace() {
    while (!IsAtEnd() && std::isspace(Peek())) {
        if (Peek() == '\n') { line_++; col_ = 0; }
        Advance();
    }
}

Token Lexer::ReadIdentifierOrKeyword() {
    int start_col = col_;
    std::string value;
    while (!IsAtEnd() && (std::isalnum(Peek()) || Peek() == '_')) {
        value += Advance();
    }

    // Check if it's a keyword (case-insensitive)
    std::string upper = value;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    auto it = keywords.find(upper);
    if (it != keywords.end()) {
        return Token(it->second, upper, line_, start_col);
    }

    return Token(TokenType::IDENTIFIER, value, line_, start_col);
}

Token Lexer::ReadNumber() {
    int start_col = col_;
    std::string value;
    bool is_float = false;

    if (Peek() == '-') value += Advance();

    while (!IsAtEnd() && std::isdigit(Peek())) {
        value += Advance();
    }

    if (!IsAtEnd() && Peek() == '.') {
        is_float = true;
        value += Advance();
        while (!IsAtEnd() && std::isdigit(Peek())) {
            value += Advance();
        }
    }

    return Token(is_float ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL,
                 value, line_, start_col);
}

Token Lexer::ReadString() {
    int start_col = col_;
    Advance(); // skip opening quote
    std::string value;
    while (!IsAtEnd() && Peek() != '\'') {
        if (Peek() == '\\') {
            Advance(); // skip escape
            if (!IsAtEnd()) value += Advance();
        } else {
            value += Advance();
        }
    }
    if (!IsAtEnd()) Advance(); // skip closing quote
    return Token(TokenType::STRING_LITERAL, value, line_, start_col);
}

char Lexer::Peek() const {
    return pos_ < input_.size() ? input_[pos_] : '\0';
}

char Lexer::Advance() {
    char c = input_[pos_++];
    col_++;
    return c;
}

bool Lexer::IsAtEnd() const {
    return pos_ >= input_.size();
}

}  // namespace minidb
