#pragma once

#include <vector>

#include "../lexer/include/token.hpp"

class SourceManager {
   public:
    SourceManager(std::vector<Token> tokens);

    const std::vector<Token>& get_src() const noexcept;
    const Token&              get(uint32_t idx) const;
    const uint32_t            size() const noexcept;

   private:
    std::vector<Token> tokens;
};
