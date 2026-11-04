#include "exception.hpp"

using namespace vm;

syntax_error::syntax_error(const char* msg, uint32_t row)
    : msg(std::string(msg) + "-> row: " + std::to_string(row)) {}

const char* syntax_error::what() const noexcept { return msg.c_str(); }
