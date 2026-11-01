#include "../include/token.hpp"

#include <algorithm>
#include <fstream>
#include <unordered_map>

Token::Token(TokenType type, const Position& pos, const std::string& lexeme)
    : type(type), pos(pos), lexeme(lexeme) {}

TokenType Token::get_type() const noexcept { return type; }

const Position& Token::get_pos() const noexcept { return pos; }

const std::string& Token::get_lexeme() const noexcept { return lexeme; }

std::ostream& operator<<(std::ostream& out, const Token& token) noexcept {
    out << "Token[Type: " << Token::type_to_str(token.type)
        << ", Position: " << token.pos.abs << ":" << token.pos.row << ":"
        << token.pos.column << ", Lexeme: " << "'" << token.lexeme << "'"
        << "]";

    return out;
}

std::string Token::type_to_str(TokenType type) noexcept {
    static std::unordered_map<TokenType, const char*> map = {
        {TokenType::KEYWORD, "KEYWORD"},
        {TokenType::IDENTIFIER, "IDENTIFIER"},
        {TokenType::OPERATOR, "OPERATOR"},
        {TokenType::NUMBER, "NUMBER"},
        {TokenType::LINE_COMMENT, "LINE_COMMENT"},
        {TokenType::SYMBOL, "SYMBOL"},
        {TokenType::STRING_SYMBOL, "STRING_SYMBOL"},
        {TokenType::DOT, "DOT"},
        {TokenType::SEMICOLON, "SEMICOLON"},
        {TokenType::UNKNOWN, "UNKNOWN"}};

    auto it = map.find(type);
    if (it == map.end()) return "UNKNOWN";

    return it->second;
}

TokenType Token::str_to_type(const std::string& s) noexcept {
    if (s.empty()) return TokenType::UNKNOWN;
    std::string str = s;
    std::transform(s.begin(), s.end(), str.begin(), ::toupper);

    static std::unordered_map<const char*, TokenType> map = {
        {"KEYWORD", TokenType::KEYWORD},
        {"IDENTIFIER", TokenType::IDENTIFIER},
        {"OPERATOR", TokenType::OPERATOR},
        {"NUMBER", TokenType::NUMBER},
        {"LINE_COMMENT", TokenType::LINE_COMMENT},
        {"SYMBOL", TokenType::SYMBOL},
        {"STRING_SYMBOL", TokenType::STRING_SYMBOL},
        {"DOT", TokenType::DOT},
        {"SEMICOLON", TokenType::SEMICOLON},
        {"UNKNOWN", TokenType::UNKNOWN}};

    auto it = map.find(str.c_str());
    if (it == map.end()) return TokenType::UNKNOWN;

    return it->second;
}
