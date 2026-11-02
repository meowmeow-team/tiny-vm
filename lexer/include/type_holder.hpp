#pragma once

#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "token_type.hpp"

class TypeHolder {
   public:
    static TokenType find(std::string_view word) noexcept;
    static TokenType find(char c) noexcept;

    static bool is_symbol(char c) noexcept;

   private:
    static const std::unordered_map<std::string_view, TokenType> src;
    static const std::unordered_set<char>                        symbols;
};
