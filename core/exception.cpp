#include "exception.hpp"

using namespace vm;

tokenizing_error::tokenizing_error(const char* msg, const char* word)
    : msg(std::string(msg) + "-> word: " + std::string(word)) {}

const char* tokenizing_error::what() const noexcept { return msg.c_str(); }

syntax_error::syntax_error(const char* msg, uint32_t row, uint32_t col)
    : msg(std::string(msg) + "-> row: " + std::to_string(row) +
          "; col: " + std::to_string(col)) {}

const char* syntax_error::what() const noexcept { return msg.c_str(); }
