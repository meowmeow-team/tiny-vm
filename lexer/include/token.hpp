#pragma once

#include <cstdint>
#include <string>

#include "position.hpp"
#include "token_type.hpp"

class Token {
   public:
    Token(TokenType type, const Position& pos, const std::string& lexeme);
    ~Token() = default;

    TokenType          get_type() const noexcept;
    const Position&    get_pos() const noexcept;
    const std::string& get_lexeme() const noexcept;

    friend std::ostream& operator<<(std::ostream& out,
                                    const Token&  token) noexcept;

    static std::string type_to_str(TokenType type) noexcept;
    static TokenType   str_to_type(const std::string& s) noexcept;

   public:
    TokenType   type;
    Position    pos;
    std::string lexeme;
};
