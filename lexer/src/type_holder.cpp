#include "../include/type_holder.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>

namespace {
bool is_num(std::string_view s) noexcept {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return std::isdigit(c);
    });
}

bool is_valid_identifier(std::string_view s) noexcept {
    if (s.empty()) return false;

    unsigned char first = static_cast<unsigned char>(s[0]);
    if (!std::isalpha(first) && first != '_') return false;

    return std::all_of(s.begin() + 1, s.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '_';
    });
}
}  // namespace

const std::unordered_map<std::string_view, TokenType> TypeHolder::src = {
    {"public", TokenType::KEYWORD},
    {"private", TokenType::KEYWORD},
    {"protected", TokenType::KEYWORD},
    {"class", TokenType::KEYWORD},
    {"if", TokenType::KEYWORD},
    {"else", TokenType::KEYWORD},
    {"while", TokenType::KEYWORD},
    {"for", TokenType::KEYWORD},
    {"void", TokenType::KEYWORD_TYPE},
    {"bool", TokenType::KEYWORD_TYPE},
    {"byte", TokenType::KEYWORD_TYPE},
    {"int", TokenType::KEYWORD_TYPE},
    {"char", TokenType::KEYWORD_TYPE},
    {"float", TokenType::KEYWORD_TYPE},
    {"double", TokenType::KEYWORD_TYPE},
    {"=", TokenType::SYMBOL_ASSIGN},
    {"+", TokenType::OPERATOR},
    {"-", TokenType::OPERATOR},
    {"*", TokenType::OPERATOR},
    {"(", TokenType::SYMBOL},
    {")", TokenType::SYMBOL},
    {"}", TokenType::SYMBOL},
    {"{", TokenType::SYMBOL},
    {"\"", TokenType::STRING_SYMBOL},
    {".", TokenType::DOT},
    {";", TokenType::SEMICOLON},
    {"==", TokenType::OPERATOR},
    {"!=", TokenType::OPERATOR},
    {"<", TokenType::OPERATOR},
    {">", TokenType::OPERATOR},
    {"&&", TokenType::OPERATOR},
    {"||", TokenType::OPERATOR},
    {"/", TokenType::OPERATOR},
    {"[", TokenType::SYMBOL},
    {"]", TokenType::SYMBOL}};

const std::unordered_set<char> TypeHolder::symbols = {'{', '}', '(', ')', '[',
                                                      ']', '"', ';', '.'};

TokenType TypeHolder::find(std::string_view word) noexcept {
    if (word.empty()) return TokenType::UNKNOWN;
    if (is_num(word)) return TokenType::NUMBER;

    auto it = src.find(word);
    if (it == src.end()) {
        if (!is_valid_identifier(word)) return TokenType::UNKNOWN;
        return TokenType::IDENTIFIER;
    }

    return it->second;
}

TokenType TypeHolder::find(char c) noexcept {
    return find(std::string_view(&c, 1));
}

bool TypeHolder::is_symbol(char c) noexcept {
    auto it = symbols.find(c);
    return it != symbols.end();
}
