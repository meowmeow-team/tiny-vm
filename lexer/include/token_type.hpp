#pragma once

enum class TokenType {
    KEYWORD,
    KEYWORD_TYPE,
    KEYWORD_RET,
    IDENTIFIER,
    OPERATOR,
    NUMBER,
    LINE_COMMENT,
    SYMBOL,
    SYMBOL_ASSIGN,
    STRING_SYMBOL,
    STRING_LITERAL,
    DOT,
    SEMICOLON,
    UNKNOWN
};
