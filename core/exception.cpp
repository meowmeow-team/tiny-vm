#include "exception.hpp"

namespace vm {

vm_error::vm_error(std::string message) : msg(std::move(message)) {}

const char* vm_error::what() const noexcept { return msg.c_str(); }

io_error::io_error(const std::string& message)
    : vm_error("IO error: " + message) {}

tokenizing_error::tokenizing_error(const char* message, const char* word)
    : vm_error(std::string(message) + " -> word: " + std::string(word)) {}

syntax_error::syntax_error(const char* message, uint32_t row, uint32_t col)
    : vm_error(std::string(message) + " -> row: " + std::to_string(row) +
               "; col: " + std::to_string(col)) {}

symbol_table_error::symbol_table_error(const char* message)
    : vm_error(message) {}

semantic_error::semantic_error(const char* message) : vm_error(message) {}

}  // namespace vm
