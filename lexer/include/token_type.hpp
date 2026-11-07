#pragma once

enum class TokenType {
    KEYWORD,
    KEYWORD_TYPE,
    IDENTIFIER,
    OPERATOR,
    NUMBER,
    LINE_COMMENT,
    SYMBOL,
    SYMBOL_ASSIGN,  // TODO
    STRING_SYMBOL,
    DOT,
    SEMICOLON,
    UNKNOWN
};
