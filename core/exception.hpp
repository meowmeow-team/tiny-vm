#pragma once

#include <cstdint>
#include <string>

namespace vm {

class tokenizing_error : public std::exception {
   public:
    tokenizing_error(const char* msg, const char* word);

    const char* what() const noexcept;

   private:
    std::string msg;
};

class syntax_error : public std::exception {
   public:
    syntax_error(const char* msg, uint32_t row);

    const char* what() const noexcept;

   private:
    std::string msg;
};

}  // namespace vm
