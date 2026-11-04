#pragma once

#include <cstdint>
#include <iostream>
#include <string>

namespace vm {

class syntax_error : public std::exception {
   public:
    syntax_error(const char* msg, uint32_t row);

    const char* what() const noexcept;

   private:
    std::string msg;
};

}  // namespace vm
