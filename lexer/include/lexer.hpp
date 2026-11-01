#pragma once

#include <set>
#include <vector>

#include "token.hpp"
#include "type_holder.hpp"

class Lexer {
   public:
    Lexer();
    ~Lexer() = default;

    std::vector<Token> load_file(const std::string& path);

    size_t get_files_lexed() const noexcept;

   private:
    TypeHolder holder;

    size_t files_lexed;
};
