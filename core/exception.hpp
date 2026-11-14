#pragma once
#include <cstdint>
#include <exception>
#include <string>

namespace vm {

class vm_error : public std::exception {
   public:
    explicit vm_error(std::string message);
    [[nodiscard]] const char* what() const noexcept override;

   protected:
    std::string msg;
};

class io_error : public vm_error {
   public:
    explicit io_error(const std::string& message);
};

class tokenizing_error : public vm_error {
   public:
    tokenizing_error(const char* message, const char* word);
};

class syntax_error : public vm_error {
   public:
    syntax_error(const char* message, uint32_t row, uint32_t col);
};

class symbol_table_error : public vm_error {
   public:
    symbol_table_error(const char* message);
};

class semantic_error : public vm_error {
   public:
    semantic_error(const char* message);
};

}  // namespace vm
